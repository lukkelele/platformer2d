#include "player.h"

#include "lk_config.h"
#include "core/log.h"
#include "core/profiler.h"
#include "core/settings.h"
#include "core/window.h"
#include "core/input/keyboard.h"
#include "renderer/renderer.h"
#include "scene/effectmanager.h"
#include "instance.h"

namespace platformer2d {

	constexpr float VELOCITY_THRESHOLD_X = CBody::LINEAR_VELOCITY_X_EPSILON;
	constexpr float VELOCITY_THRESHOLD_Y = CBody::LINEAR_VELOCITY_Y_EPSILON;

	static constexpr std::array<EKey, 5> MovementKeys = {
		EKey::W,
		EKey::A,
		EKey::S,
		EKey::D,
		EKey::Space};

	CPlayer::CPlayer(const FActorSpecification& InSpec, const FBodySpecification& BodySpec)
		: CActor(InSpec, BodySpec)
		, Inventory("PlayerInventory")
	{
		if (Name.empty()) {
			Name = "Player";
		}

		FCameraComponent& CamComp = AddComponent<FCameraComponent>();
		CamComp.Camera = std::make_shared<CCamera>(SCREEN_WIDTH, SCREEN_HEIGHT);
		AddComponent<FHealthComponent>();
		SetDeletable(false);

		LK_VERIFY(InSpec.Texture == ETexture::Player, "Player texture mismatch: {}", Enum::ToString(InSpec.Texture));

		FSpriteReader Reader;
		std::optional<FSpriteSheet> LoadedSheet = Reader.Read(TEXTURES_DIR "/sprites/Player.lsprite");
		LK_VERIFY(LoadedSheet, "Failed to read Player.lsprite");
		SpriteSheet = std::move(*LoadedSheet);

		const FSpriteCoord InitialFrame = SpriteSheet.Get(ESpriteFrame::Idle).First();
		SpriteFrame.Current = InitialFrame;
		SpriteFrame.Next = InitialFrame;

		const glm::vec2 TilePos{InitialFrame.X, InitialFrame.Y};
		Sprite = std::make_unique<CSprite>(CRenderer::GetTexture(Texture), TilePos, SpriteSheet.TileSize);

		Timer.Reset();
		LK_VERIFY(Body && Sprite && CamComp.Camera);

		/* Set z-index. */
		TransformComp.Translation.z = -0.010f;

#ifdef SPAWN_WITH_RIFLE
		std::shared_ptr<CRifle> Rifle = std::make_shared<CRifle>();
		Rifle->Equip(this);
		Inventory.AddItem(Rifle);
#endif
	}

	CPlayer::~CPlayer()
	{
		LK_DEBUG_TAG("Player", "Release: {}", GetName());
		Inventory.Destroy();
		Inventory.~CInventory();
	}

	void CPlayer::Tick(const float DeltaTime)
	{
		LK_PROFILE_FUNC();
		CActor::Tick(DeltaTime);

		if (DeltaTime > 0.0f) {
			CheckCollisions();
			UpdateMovementState();

			if (bShouldUpdateSprite) {
				UpdateSprite();
			}

			HandleInput();
			SyncTransformComponent();
		}

		Inventory.Tick(DeltaTime);

		CCamera& Camera = GetCamera();
		const float CamDt = (DeltaTime > 0.0f ? DeltaTime : 0.0060f);
		if (Camera.IsSwitchLerping()) {
			Camera.SetSwitchTargetPos(Body->GetPosition());
			Camera.TickSwitchLerp(CamDt);
		} else if (bCameraLock) {
			/* Perform a smooth transition for the target lock even if paused. */
			Camera.Target(Body->GetPosition(), CamDt);
		}
		Camera.Update();
	}

	void CPlayer::Jump()
	{
		if (!Data.bJumping) {
			Data.bJumping = true;
			SetMovementState(EMovementState::Airborne);
			Body->ApplyImpulse({0.0f, JumpImpulse});

			SpriteFrame.Next = SpriteSheet.Get(ESpriteFrame::Jump).First();
			CEffectManager::Get().Play(EEffect::Swoosh, GetPosition(), 220ms);

			OnJumped.Broadcast(Data);
		}
	}

	void CPlayer::OnDeath()
	{
		LK_INFO_TAG("Player", "[{}] OnDeath", GetName());
		CEffectManager::Get().Play(EEffect::Swoosh, GetPosition(), 300ms);
		if (Body) {
			Body->SetLinearVelocity({0.0f, 0.0f});
			Body->SetEnabled(false);
		}
		OnDied.Broadcast(Data);
	}

	void CPlayer::SetJumpImpulse(const float Impulse)
	{
		JumpImpulse = Impulse;
	}

	void CPlayer::SetDirectionForce(const float Force)
	{
		DirForce = Force;
	}

	void CPlayer::SetAwake(const bool Awake) const
	{
		Body->SetAwake(Awake);
	}

	void CPlayer::SetLookDirection(const EDirection InDirection)
	{
		if (LookDir != InDirection) {
			LookDir = InDirection;
			ForceUpdateSprite();
		}
	}

	void CPlayer::SetCameraLock(const bool Locked)
	{
		bCameraLock = Locked;
	}

	bool CPlayer::HasRifle()
	{
		return (Inventory.FindFirstOf<CRifle>() != nullptr);
	}

	std::shared_ptr<CRifle> CPlayer::GetRifle()
	{
		return Inventory.FindFirstOf<CRifle>();
	}

	void CPlayer::SetClimbZone(const bool InZone, const float InClimbSpeed)
	{
		bInClimbZone = InZone;
		ClimbSpeed = InZone ? InClimbSpeed : 0.0f;
	}

	bool CPlayer::Serialize(YAML::Emitter& Out, const EExtendableSerializer Extendable) const
	{
		LK_UNUSED(Extendable);
		LK_DEBUG_TAG("Player", "Serialize");
		CActor::Serialize(Out, EExtendableSerializer::Yes);
		Out << YAML::EndMap;

		return true;
	}

	void CPlayer::HandleInput()
	{
		if (CKeyboard::IsKeyDown(EKey::W)) {
			bWantToClimb = true;
			LastDirForce = 0.0f;
			OnInputReceived();
		}
		if (CKeyboard::IsKeyDown(EKey::A)) {
			Body->ApplyForce({-DirForce, 0.0f});
			LookDir = EDirection::Left;
			LastDirForce = -DirForce;
			OnInputReceived();
		}
		if (CKeyboard::IsKeyDown(EKey::D)) {
			Body->ApplyForce({DirForce, 0.0f});
			LastDirForce = DirForce;
			LookDir = EDirection::Right;
			OnInputReceived();
		}
		if (CKeyboard::IsKeyDown(EKey::Space)) {
			Jump();
			LastDirForce = 0.0f;
			OnInputReceived();
		}

		/* Clear movement input flag if needed. */
		if (bMovementInputLastTick && !CKeyboard::IsAnyKeysDown(MovementKeys)) {
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
		if (bJustLanded) {
			LK_TRACE_TAG("Player", "Just landed");
			/* The idle state will get evaluated to idle/running later. */
			SetMovementState(EMovementState::Idle);
			bJustLanded = false;
		}

		if (bWantToClimb) {
			if (!IsMoving()) {
				/* @todo: Begin climbing if possible */
			}
			bWantToClimb = false;
		}

		switch (Data.MovementState) {
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

		bShouldUpdateSprite = (SpriteFrame.Current != SpriteFrame.Next);
	}

	void CPlayer::MovementState_Idle()
	{
		const glm::vec2 LinearVelocity = Body->GetLinearVelocity();
		const bool MovingByInput = (LastDirForce != 0.0f);

		if (std::abs(LinearVelocity.x) > VELOCITY_THRESHOLD_X) {
			if (MovingByInput) {
				SetMovementState(EMovementState::Running);
			} else {
				SpriteFrame.Next = SpriteSheet.Get(ESpriteFrame::Hit).First();
			}
		} else {
			const auto TimeNow = std::chrono::steady_clock::now();
			if (TimeNow - LastInputTime > 150ms) {
				SpriteFrame.Next = SpriteSheet.Get(ESpriteFrame::Idle).First();
			}
		}
	}

	void CPlayer::MovementState_Running()
	{
		const glm::vec2 LinearVelocity = Body->GetLinearVelocity();
		const std::uint16_t FrameIndex = CRenderer::GetFrameIndex();
		const bool MovingByInput = (LastDirForce != 0.0f);

		if (std::abs(LinearVelocity.x) > VELOCITY_THRESHOLD_X) {
			SpriteFrame.Next = SpriteSheet.Get(ESpriteFrame::Walk).GetFrame(FrameIndex);
		} else if (!MovingByInput && std::abs(LinearVelocity.x) < VELOCITY_THRESHOLD_X) {
			SetMovementState(EMovementState::Idle);
			SpriteFrame.Next = SpriteSheet.Get(ESpriteFrame::Idle).First();
		}
	}

	void CPlayer::MovementState_Airborne()
	{
		const glm::vec2 LinearVelocity = Body->GetLinearVelocity();

		if (LinearVelocity.y > VELOCITY_THRESHOLD_Y) {
			SpriteFrame.Next = SpriteSheet.Get(ESpriteFrame::JumpAscend).First();
		} else if (LinearVelocity.y < -VELOCITY_THRESHOLD_Y) {
			SpriteFrame.Next = SpriteSheet.Get(ESpriteFrame::JumpDescend).First();
		}
	}

	void CPlayer::SetMovementState(const EMovementState State)
	{
		if (Data.MovementState == State) {
			return;
		}

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
		for (int Idx = 0; Idx < Count; Idx++) {
			b2BodyId BodyA = b2Shape_GetBody(ContactData[Idx].shapeIdA);
			const float Sign = (B2_ID_EQUALS(BodyA, BodyID)) ? -1.0f : 1.0f;
			if (Sign * ContactData[Idx].manifold.normal.y > 0.90f) {
				bCanJump = true;
				break;
			}
		}

		if (bCanJump && Data.bJumping) {
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
		SetSpriteTilePos(SpriteFrame.Next);
		LK_ASSERT(SpriteFrame.Current == SpriteFrame.Next);
	}

	void CPlayer::ForceUpdateSprite()
	{
		SetSpriteTilePos(SpriteFrame.Next, true);
		LK_ASSERT(SpriteFrame.Current == SpriteFrame.Next);
	}

	void CPlayer::SetSpriteTilePos(const FSpriteCoord& InCoord, const bool ForceUpdate)
	{
		if (ForceUpdate || (SpriteFrame.Current != InCoord)) {
			const bool FlipHorizontal = (LookDir == EDirection::Left);
			Sprite->SetTilePos(InCoord.X, InCoord.Y, FlipHorizontal);
			SpriteFrame.Current = InCoord;

			if (std::shared_ptr<CRifle> Rifle = Inventory.FindFirstOf<CRifle>()) {
				Rifle->SetLookDirection(LookDir);
			}
		}
	}

	void CPlayer::OnWindowResized(const std::uint16_t Width, const std::uint16_t Height)
	{
		if (auto* CamComp = TryGetComponent<FCameraComponent>(); CamComp && CamComp->Camera) {
			LK_TRACE_TAG("Player", "Window resized: ({}, {})", Width, Height);
			CCamera& Camera = *CamComp->Camera;
			Camera.SetViewportSize(Width, Height);
			Camera.UpdateView();
			Camera.UpdateProjection();
		}
	}

	void CPlayer::OnKey(const FKeyData& Data)
	{
		switch (Data.Key) {
			case EKey::Q:
				break;

			case EKey::F:
				break;

			case EKey::R:
				if (std::shared_ptr<CRifle> Rifle = Inventory.FindFirstOf<CRifle>()) {
					Rifle->Reload();
				}
				break;

			case EKey::V:
				if (Data.State == EKeyState::Pressed) {
					/* Toggle rifle. */
					if (std::shared_ptr<CRifle> Rifle = Inventory.FindFirstOf<CRifle>()) {
						Rifle->SetEnabled(!Rifle->IsEnabled());
					}
				}
				break;
		}
	}

	void CPlayer::OnMouseButton(const FMouseButtonData& Data)
	{
		switch (Data.Button) {
			case EMouseButton::Button0:
			{
				if (Data.State == EMouseButtonState::Pressed) {
					std::shared_ptr<CRifle> Rifle = Inventory.FindFirstOf<CRifle>();
					if (Rifle && Rifle->IsEnabled()) {
						auto& GameInstance = CGameInstance::Get();
						if (CCamera* Camera = GameInstance.GetActiveCamera()) {
							const glm::vec2 TargetPos = GameInstance.GetMouseInWorldSpace(*Camera);
							if (Math::IsValid(TargetPos)) {
								Rifle->Fire(TargetPos);
							}
						}
					}
				}
			}
		}
	}

	void CPlayer::OnMouseScroll(const EMouseScrollDirection Direction)
	{
		LK_TRACE_TAG("Player", "Mouse scroll: {}", Enum::ToString(Direction));
		if (CKeyboard::IsAnyKeysDown(EKey::LeftControl, EKey::RightControl)) {
			auto& CamComp = GetComponent<FCameraComponent>();
			CCamera& Camera = *CamComp.Camera;
			const float ZoomSpeed = std::clamp(FSettings::Get().Input.ZoomSpeed, 0.050f, 8.0f);
			const float Step = CCamera::ZOOM_DIFF * ZoomSpeed;
			const float ZoomDiff = (Direction == EMouseScrollDirection::Up) ? -Step : Step;
			Camera.SetZoom(Camera.GetZoom() + ZoomDiff);
		}
	}

}
