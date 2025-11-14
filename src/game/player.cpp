#include "player.h"

#include "core/log.h"
#include "core/window.h"
#include "core/input/keyboard.h"
#include "renderer/renderer.h"
#include "scene/effectmanager.h"

namespace platformer2d {

	namespace
	{
		constexpr int SPRITE_TILEPOS_Y = 2; /* Row in the spritesheet. */

		enum class ESpriteFrame : uint16_t
		{
			/* Cycle: 0->1->2->3 */
			WalkStart = 0,
			Walk1 = 1,
			Walk2 = 2,
			WalkEnd = 3,

			JumpPreparation = 4,
			JumpAscend = 5,
			JumpDescend = 6,
			JumpLanding = 7,

			/* Cycle: 8->9->8 */
			Hit1 = 8,
			Hit2 = 9,

			/* 10->11->12 (With preparation: 11->10->11->12) */
			Slash1 = 10,
			Slash2 = 11,
			Slash3 = 12,

			/* 13->11 (Variation: 11->13->11) */
			Punch1 = 11,
			Punch2 = 13,

			WalkReversedStart = 18,
			WalkReversed1 = 19,
			WalkReversed2 = 20,
			WalkReversedEnd = 21,

			COUNT
		};

		constexpr float VelocityThresholdX = CBody::LINEAR_VELOCITY_X_EPSILON;
		constexpr float VelocityThresholdY = CBody::LINEAR_VELOCITY_Y_EPSILON;

		constexpr std::array<EKey, 5> MovementKeys = {
			EKey::W,
			EKey::A,
			EKey::S,
			EKey::D,
			EKey::Space
		};
	}

	CPlayer::CPlayer(const FActorSpecification& Spec)
		: CActor(Spec)
		, NextSpriteFrame(std::to_underlying(ESpriteFrame::COUNT))
	{
		LK_VERIFY(Spec.Texture == ETexture::Player, "Player texture mismatch: {}", Enum::ToString(Spec.Texture));
		Camera = std::make_unique<CCamera>(SCREEN_WIDTH, SCREEN_HEIGHT);
		CWindow::OnResized.Add(this, &CPlayer::OnWindowResized);
		CMouse::OnScrolled.Add(this, &CPlayer::OnMouseScrolled);

		if (Name.empty())
		{
			Name = "Player";
		}

		constexpr glm::vec2 TilePos = { ESpriteFrame::WalkStart, SPRITE_TILEPOS_Y };
		WalkAnim.StartTileX = TilePos.x;
		WalkAnim.StartTileY = TilePos.y;
		WalkAnim.FrameCount = 4;
		WalkAnim.TicksPerFrame = WalkAnim.FrameCount * 4;
		constexpr glm::vec2 TileSize = { 32, 32 };
		Sprite = std::make_unique<CSprite>(CRenderer::GetTexture(Texture), TilePos, TileSize);

		Timer.Reset();
		LK_VERIFY(Body && Sprite);
	}

	CPlayer::CPlayer(const FBodySpecification& BodySpec, const ETexture InTexture)
		: CActor(BodySpec, InTexture)
		, NextSpriteFrame(std::to_underlying(ESpriteFrame::COUNT))
	{
		Camera = std::make_unique<CCamera>(SCREEN_WIDTH, SCREEN_HEIGHT);
		CWindow::OnResized.Add(this, &CPlayer::OnWindowResized);
		CMouse::OnScrolled.Add(this, &CPlayer::OnMouseScrolled);

		if (Name.empty())
		{
			Name = "Player";
		}

		constexpr glm::vec2 TilePos = { ESpriteFrame::WalkStart, SPRITE_TILEPOS_Y };
		WalkAnim.StartTileX = TilePos.x;
		WalkAnim.StartTileY = TilePos.y;
		WalkAnim.FrameCount = 4;
		WalkAnim.TicksPerFrame = WalkAnim.FrameCount * 5;
		constexpr glm::vec2 TileSize = { 32, 32 };
		LK_VERIFY(InTexture == ETexture::Player, "Player texture mismatch: {}", Enum::ToString(InTexture));
		Sprite = std::make_unique<CSprite>(CRenderer::GetTexture(Texture), TilePos, TileSize);

		Timer.Reset();
		LK_VERIFY(Body && Sprite);
	}

	void CPlayer::Tick(const float DeltaTime)
	{
		CActor::Tick(DeltaTime);

		CheckCollisions();
		UpdateMovementState();
		if (bShouldUpdateSprite)
		{
			UpdateSprite();
		}

		HandleInput();
		SyncTransformComponent();

		if (bCameraLock)
		{
			Camera->Target(Body->GetPosition(), DeltaTime);
		}
		Camera->Update();
	}

	void CPlayer::Jump()
	{
		if (!Data.bJumping)
		{
			Data.bJumping = true;
			SetMovementState(EMovementState::Airborne);
			Body->ApplyImpulse({ 0.0f, JumpImpulse });

			NextSpriteFrame = Enum::AsUnderlying(ESpriteFrame::JumpPreparation);
			CEffectManager::Get().Play(EEffect::Swoosh, GetPosition(), 220ms);

			OnJumped.Broadcast(Data);
		}
	}

	void CPlayer::SetJumpImpulse(const float Impulse)
	{
		JumpImpulse = Impulse;
	}

	void CPlayer::SetDirectionForce(const float Force)
	{
		DirForce = Force;
	}

	void CPlayer::SetCameraLock(const bool Locked)
	{
		bCameraLock = Locked;
	}

	glm::vec2 CPlayer::GetSize() const
	{
		glm::vec2 Size = Body->GetSize();
		Size *= glm::vec2(TransformComp.Scale.x, TransformComp.Scale.y);
		return Size;
	}

	void CPlayer::HandleInput()
	{
		if (CKeyboard::IsKeyDown(EKey::W))
		{
			bWantToClimb = true;
			LastDirForce = 0.0f;
			OnInputReceived();
		}
		if (CKeyboard::IsKeyDown(EKey::A))
		{
			WalkAnim.StartTileX = Enum::AsUnderlying(ESpriteFrame::WalkStart);
			Body->ApplyForce({ -DirForce, 0.0f });
			LookDir = EDirection::Left;
			LastDirForce = -DirForce;
			OnInputReceived();
		}
		if (CKeyboard::IsKeyDown(EKey::D))
		{
			WalkAnim.StartTileX = Enum::AsUnderlying(ESpriteFrame::WalkStart);
			Body->ApplyForce({ DirForce, 0.0f });
			LastDirForce = DirForce;
			LookDir = EDirection::Right;
			OnInputReceived();
		}
		if (CKeyboard::IsKeyDown(EKey::Space))
		{
			WalkAnim.StartTileX = Enum::AsUnderlying(ESpriteFrame::JumpPreparation);
			Jump();
			LastDirForce = 0.0f;
			OnInputReceived();
		}

		/* Clear movement input flag if needed. */
		if (bMovementInputLastTick && !CKeyboard::IsAnyKeysDown(MovementKeys))
		{
			LK_TRACE_TAG("Player", "Clear movement input tick flag");
			LastDirForce = 0.0f;
			bMovementInputLastTick = false;
		}
	}

	void CPlayer::OnInputReceived()
	{
		bMovementInputLastTick = true;
		LastInputTime = std::chrono::steady_clock::now();
	}

	void CPlayer::UpdateMovementState()
	{
		if (bJustLanded)
		{
			LK_TRACE_TAG("Player", "Just landed");
			/* The idle state will get evaluated to idle/running later. */
			SetMovementState(EMovementState::Idle);
			bJustLanded = false;
		}

		if (bWantToClimb)
		{
			if (!IsMoving())
			{
				/* @todo: Begin climbing if possible */
			}
			bWantToClimb = false;
		}

		switch (Data.MovementState)
		{
			case EMovementState::Idle:
				MovementState_Idle();
				break;

			case EMovementState::Running:
				MovementState_Running();
				break;

			case EMovementState::Airborne:
				MovementState_Airborne();
				break;
		}

		bShouldUpdateSprite = (CurrentSpriteFrame != NextSpriteFrame);
	}

	void CPlayer::MovementState_Idle()
	{
		const glm::vec2 LinearVelocity = Body->GetLinearVelocity();
		const bool MovingByInput = (LastDirForce != 0.0f);

		/* Movement in X-axis */
		if (std::abs(LinearVelocity.x) > VelocityThresholdX)
		{
			if (MovingByInput)
			{
				SetMovementState(EMovementState::Running);
				WalkAnim.StartTileX = Enum::AsUnderlying(ESpriteFrame::WalkStart);
			}
			else
			{
				/* Player is moving because of external forces. */
				NextSpriteFrame = Enum::AsUnderlying(ESpriteFrame::Hit1);
			}
		}
		else
		{
			/* Player character is idle. */
			const auto TimeNow = std::chrono::steady_clock::now();
			if (TimeNow - LastInputTime > 150ms)
			{
				/* Always turn the player character frontward after a brief delay. */
				WalkAnim.StartTileX = Enum::AsUnderlying(ESpriteFrame::WalkStart);
				NextSpriteFrame = WalkAnim.StartTileX;
			}
		}
	}

	void CPlayer::MovementState_Running()
	{
		const glm::vec2 LinearVelocity = Body->GetLinearVelocity();
		const uint16_t FrameIndex = CRenderer::GetFrameIndex();
		const bool MovingByInput = (LastDirForce != 0.0f);

		if (std::abs(LinearVelocity.x) > VelocityThresholdX)
		{
			const uint16_t AnimFrame = WalkAnim.CalculateAnimFrame(FrameIndex);
			NextSpriteFrame = AnimFrame;
		}
		else if (!MovingByInput && std::abs(LinearVelocity.x) < VelocityThresholdX)
		{
			/* Player is idle. */
			SetMovementState(EMovementState::Idle);
			WalkAnim.StartTileX = Enum::AsUnderlying(ESpriteFrame::WalkStart);
			NextSpriteFrame = WalkAnim.StartTileX;
		}
	}

	void CPlayer::MovementState_Airborne()
	{
		const glm::vec2 LinearVelocity = Body->GetLinearVelocity();

		/* Check if ascending or descending. */
		if (LinearVelocity.y > VelocityThresholdY)
		{
			NextSpriteFrame = Enum::AsUnderlying(ESpriteFrame::JumpAscend);
		}
		else if (LinearVelocity.y < -VelocityThresholdY)
		{
			NextSpriteFrame = Enum::AsUnderlying(ESpriteFrame::JumpDescend);
		}
	}

	void CPlayer::SetMovementState(const EMovementState State)
	{
		if (Data.MovementState == State)
		{
			return;
		}

		LK_TRACE_TAG("Player", "{} -> {}", Enum::ToString(Data.MovementState), Enum::ToString(State));
		Data.MovementState = State;
	}

	void CPlayer::CheckCollisions()
	{
		static constexpr int MAX_CONTACTS = 4;
		const b2BodyId BodyID = Body->GetID();
		const int Capacity = std::min(b2Body_GetContactCapacity(BodyID), MAX_CONTACTS);

		bool bCanJump = false;
		b2ContactData ContactData[MAX_CONTACTS];
		const int Count = b2Body_GetContactData(BodyID, ContactData, Capacity);
		for (int Idx = 0; Idx < Count; Idx++)
		{
			b2BodyId BodyA = b2Shape_GetBody(ContactData[Idx].shapeIdA);
			const float Sign = (B2_ID_EQUALS(BodyA, BodyID)) ? -1.0f : 1.0f;
			if (Sign * ContactData[Idx].manifold.normal.y > 0.90f)
			{
				bCanJump = true;
				break;
			}
		}

		if (bCanJump && Data.bJumping)
		{
			Data.bJumping = false;
			bJustLanded = true;
			OnLanded.Broadcast(Data);
		}
	}

	void CPlayer::SyncTransformComponent()
	{
		const glm::vec2 BodySize = Body->GetSize();
		TransformComp.Scale.x = BodySize.x;
		TransformComp.Scale.y = BodySize.y;
	}

	void CPlayer::UpdateSprite()
	{
		SetSpriteTilePos(NextSpriteFrame);
		LK_ASSERT(CurrentSpriteFrame == NextSpriteFrame);
	}

	void CPlayer::SetSpriteTilePos(const uint16_t X)
	{
		if (CurrentSpriteFrame != X)
		{
			const bool FlipHorizontal = (LookDir == EDirection::Left);
			Sprite->SetTilePos(X, SPRITE_TILEPOS_Y, FlipHorizontal);
			CurrentSpriteFrame = X;
		}
	}

	void CPlayer::OnWindowResized(const uint16_t Width, const uint16_t Height)
	{
		if (Camera)
		{
			LK_TRACE_TAG("Player", "Window resized: ({}, {})", Width, Height);
			Camera->SetViewportSize(Width, Height);
			Camera->UpdateView();
			Camera->UpdateProjection();
		}
	}

	void CPlayer::OnMouseScrolled(const EMouseScrollDirection Direction)
	{
		LK_TRACE_TAG("Player", "Mouse scroll: {}", Enum::ToString(Direction));
		if (CKeyboard::IsKeyDown(EKey::LeftControl) || CKeyboard::IsKeyDown(EKey::RightControl))
		{
			if (Camera)
			{
				const float ZoomDiff = (Direction == EMouseScrollDirection::Up)
					? -CCamera::ZOOM_DIFF
					: CCamera::ZOOM_DIFF;
				Camera->SetZoom(Camera->GetZoom() + ZoomDiff);
			}
		}
	}

}
