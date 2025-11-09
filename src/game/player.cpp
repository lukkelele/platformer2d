#include "player.h"

#include "core/log.h"
#include "core/window.h"
#include "core/input/keyboard.h"
#include "renderer/renderer.h"

namespace platformer2d {

	namespace
	{
		/** @todo: The Y tilepos does not start from index 0... */
		constexpr int SPRITE_TILEPOS_Y = 2; /* Second row in the spritesheet. */

		enum class ESpriteFrame : uint16_t
		{
			/* Cycle: 1->2->3->4 */
			WalkStart = 1,
			Walk2 = 2,
			Walk3 = 3,
			WalkEnd = 4,

			JumpPreparation = 5,
			JumpAscend = 6,
			JumpDescend = 7,
			JumpLanding = 8,

			/* Cycle: 9->10->9 */
			Hit1 = 9,
			Hit2 = 10,

			/* 11->12->13 (With preparation: 12->11->12->13) */
			Slash1 = 11,
			Slash2 = 12,
			Slash3 = 13,

			/* 14->12 (Variation: 12->14->12) */
			Punch1 = 12,
			Punch2 = 14,

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

	CPlayer::CPlayer(const FActorSpecification& Spec, const ETexture InTexture)
		: CActor(Spec, InTexture)
		, NextSpriteFrame(Enum::AsUnderlying(ESpriteFrame::COUNT))
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
		WalkAnim.TicksPerFrame = 18;
		constexpr glm::vec2 TileSize = { 32, 32 };
		LK_VERIFY(InTexture == ETexture::Player, "Player texture mismatch: {}", Enum::ToString(InTexture));
		Sprite = std::make_unique<CSprite>(CRenderer::GetTexture(Texture), TilePos, TileSize);

		Timer.Reset();
		LK_VERIFY(Body && Sprite);
	}

	CPlayer::CPlayer(const FBodySpecification& BodySpec, const ETexture InTexture)
		: CActor(BodySpec, InTexture)
		, NextSpriteFrame(Enum::AsUnderlying(ESpriteFrame::COUNT))
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
		WalkAnim.TicksPerFrame = 24;
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
			Data.MovementState = EMovementState::Airborne;
			Body->ApplyImpulse({ 0.0f, JumpImpulse });
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
		if (CKeyboard::IsKeyDown(EKey::A))
		{
			Body->ApplyForce({ -DirForce, 0.0f });
			LookDir = EDirection::Left;
			LastDirForce = -DirForce;
			bMovementInputLastTick = true;
		}
		if (CKeyboard::IsKeyDown(EKey::D))
		{
			Body->ApplyForce({ DirForce, 0.0f });
			LookDir = EDirection::Right;
			LastDirForce = DirForce;
			bMovementInputLastTick = true;
		}
		if (CKeyboard::IsKeyDown(EKey::Space))
		{
			Jump();
			LastDirForce = 0.0f;
			bMovementInputLastTick = true;
		}

		/* Clear movement input flag if needed. */
		if (bMovementInputLastTick && !CKeyboard::IsAnyKeysDown(MovementKeys))
		{
			LK_TRACE_TAG("Player", "Clear movement input tick flag");
			LastDirForce = 0.0f;
			bMovementInputLastTick = false;
		}
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
				NextSpriteFrame = Enum::AsUnderlying(ESpriteFrame::WalkStart);
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
			NextSpriteFrame = Enum::AsUnderlying(ESpriteFrame::WalkStart);
		}
	}

	void CPlayer::MovementState_Running()
	{
		const glm::vec2 LinearVelocity = Body->GetLinearVelocity();
		const uint16_t FrameIndex = CRenderer::GetFrameIndex();
		const bool MovingByInput = (LastDirForce != 0.0f);

		if (std::abs(LinearVelocity.x) > VelocityThresholdX)
		{
			/* Increment the frame by 1 because the sprite sheet begins at position 1. */
			const uint16_t AnimFrame = WalkAnim.CalculateAnimFrame(FrameIndex) + 1;
			NextSpriteFrame = AnimFrame;
		}
		else if (!MovingByInput && std::abs(LinearVelocity.x) < VelocityThresholdX)
		{
			SetMovementState(EMovementState::Idle);
			NextSpriteFrame = Enum::AsUnderlying(ESpriteFrame::WalkStart);
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
