#include "player.h"

#include "lk_config.h"
#include "core/log.h"
#include "core/profiler.h"
#include "core/settings.h"
#include "core/window.h"
#include "core/input/keyboard.h"
#include "renderer/renderer.h"
#include "scene/effectmanager.h"
#include "inputsystem.h"
#include "instance.h"
#include "projectilesystem.h"
#include "physics/physicsworld.h"

namespace platformer2d {

	constexpr float VELOCITY_THRESHOLD_X = CBody::LINEAR_VELOCITY_X_EPSILON;
	constexpr float VELOCITY_THRESHOLD_Y = CBody::LINEAR_VELOCITY_Y_EPSILON;

	static constexpr std::array<EAction, 5> MovementActions = {
		EAction::MoveUp,
		EAction::MoveLeft,
		EAction::MoveRight,
		EAction::MoveDown,
		EAction::Jump};

	CPlayer::CPlayer(const FActorSpecification& InSpec, const FBodySpecification& BodySpec)
		: CActor(InSpec, BodySpec)
		, Inventory("PlayerInventory")
		, DirForce(BodySpec.DirForce)
		, JumpImpulse(BodySpec.JumpImpulse)
	{
		LK_VERIFY(InSpec.Texture == ETexture::Player, "Player texture mismatch: {}", Enum::ToString(InSpec.Texture));
		if (Name.empty()) {
			Name = "Player";
		}

		if (Body) {
			Body->SetCollisionCategory(ECollisionCategory_Player);
		}

		FCameraComponent& CamComp = AddComponent<FCameraComponent>();
		CamComp.Camera = std::make_shared<CCamera>(SCREEN_WIDTH, SCREEN_HEIGHT);
		AddComponent<FHealthComponent>();
		SetDeletable(false);

		SpriteSheet = CRenderer::GetSpriteSheet(Texture);
		LK_ASSERT(SpriteSheet, "Sprite sheet not loaded: {}", Enum::ToString(Texture));
		const FSpriteCoord InitialFrame = SpriteSheet->Get(ESpriteFrame::Idle).First();
		SpriteFrame.Current = InitialFrame;
		SpriteFrame.Next = InitialFrame;

		const glm::vec2 TilePos{InitialFrame.X, InitialFrame.Y};
		Sprite = std::make_unique<CSprite>(CRenderer::GetTexture(Texture), TilePos, SpriteSheet->TileSize);

		Timer.Reset();
		LK_VERIFY(Body && Sprite && CamComp.Camera);

		/* Set z-index. */
		TransformComp.Translation.z = -0.010f;

#ifdef SPAWN_WITH_RIFLE
		std::shared_ptr<CRifle> Rifle = std::make_shared<CRifle>();
		Rifle->Equip(this);
		Inventory.AddItem(Rifle);
#endif

#ifdef SPAWN_WITH_MELEE
		std::shared_ptr<CMelee> Melee = std::make_shared<CMelee>();
		Melee->Equip(this);
		Inventory.AddItem(Melee);
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
		LK_PROFILER_SCOPED();
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

			SpriteFrame.Next = SpriteSheet->Get(ESpriteFrame::Jump).First();
			const float JumpSwooshDirX = (LookDir == EDirection::Left) ? 0.40f : -0.40f;
			CEffectManager::Get().Play(EEffect::Swoosh, GetPosition(), 220ms,
				{0.15f, 0.15f}, 1.0f, {JumpSwooshDirX, -0.60f});

			OnJumped.Broadcast(Data);
		}
	}

	void CPlayer::OnDeath(const EDeathReason Reason)
	{
		LK_INFO_TAG("Player", "[{}] OnDeath: {}", GetName(), Enum::ToString(Reason));
		CEffectManager::Get().Play(EEffect::Swoosh, GetPosition(), 300ms,
			{0.15f, 0.15f}, 1.0f, {0.0f, 0.40f});
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

	void CPlayer::SetSpriteSheet(const ETexture InTexture)
	{
		const FSpriteSheet* Sheet = CRenderer::GetSpriteSheet(InTexture);
		LK_ASSERT(Sheet);
		LK_VERIFY(Sheet->Has(ESpriteFrame::Idle), "{}: Missing idle", Enum::ToString(InTexture));
		LK_DEBUG_TAG("Player", "Swapping sprite sheet: {} -> {}", Enum::ToString(Texture), Enum::ToString(InTexture));
		Texture = InTexture;
		SpriteSheet = Sheet;

		const FSpriteCoord InitialFrame = SpriteSheet->Get(ESpriteFrame::Idle).First();
		SpriteFrame.Current = InitialFrame;
		SpriteFrame.Next = InitialFrame;

		const glm::vec2 TilePos{InitialFrame.X, InitialFrame.Y};
		Sprite = std::make_unique<CSprite>(CRenderer::GetTexture(Texture), TilePos, SpriteSheet->TileSize);
		bShouldUpdateSprite = true;
	}

	bool CPlayer::HasRifle()
	{
		return (Inventory.FindFirstOf<CRifle>() != nullptr);
	}

	std::shared_ptr<CRifle> CPlayer::GetRifle()
	{
		return Inventory.FindFirstOf<CRifle>();
	}

	bool CPlayer::CanThrowProjectile() const
	{
		return std::chrono::steady_clock::now() >= NextProjectileTime;
	}

	void CPlayer::ThrowProjectile()
	{
		if (!CanThrowProjectile()) {
			return;
		}

		const float DirSign = (LookDir == EDirection::Left) ? -1.0f : 1.0f;

		FProjectileSpawnParams Params;
		Params.Spawner = this;
		Params.Position = {GetPosition().x + (DirSign * 0.080f), GetPosition().y + 0.020f};
		Params.Velocity = {DirSign * ProjectileThrowSpeed, ProjectileThrowSpeed * 0.55f};
		Params.Radius = ProjectileRadius;
		Params.Restitution = ProjectileRestitution;
		Params.Damage = ProjectileDamage;
		Params.Color = FColor::Red;
		Params.MaxBounceCount = 1;
		Params.bExplodeOnImpact = false;
		Params.RenderZ = -0.020f;
		Params.ExpireTimeout = ProjectileExpireTimeout;
		Params.NamePrefix = "Ball";

		CGameInstance::Get().GetSystem<CProjectileSystem>().Spawn(Params);

		NextProjectileTime = std::chrono::steady_clock::now() + ProjectileCooldown;
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
		const CInputSystem& Input = CGameInstance::Get().GetSystem<CInputSystem>();

		if (Input.IsActionDown(EAction::MoveUp)) {
			bWantToClimb = true;
			LastDirForce = 0.0f;
			OnInputReceived();
		}
		if (Input.IsActionDown(EAction::MoveLeft)) {
			Body->ApplyForce({-DirForce, 0.0f});
			LookDir = EDirection::Left;
			LastDirForce = -DirForce;
			OnInputReceived();
		}
		if (Input.IsActionDown(EAction::MoveRight)) {
			Body->ApplyForce({DirForce, 0.0f});
			LastDirForce = DirForce;
			LookDir = EDirection::Right;
			OnInputReceived();
		}
		if (Input.IsActionDown(EAction::Jump)) {
			Jump();
			LastDirForce = 0.0f;
			OnInputReceived();
		}

		/* Clear movement input flag if needed. */
		const bool AnyMovementInput = std::ranges::any_of(MovementActions, [&Input](const EAction Action)
		{
			return Input.IsActionDown(Action);
		});
		if (bMovementInputLastTick && !AnyMovementInput) {
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
				SpriteFrame.Next = SpriteSheet->Get(ESpriteFrame::Hit).First();
			}
		} else {
			const auto TimeNow = std::chrono::steady_clock::now();
			if (TimeNow - LastInputTime > 150ms) {
				SpriteFrame.Next = SpriteSheet->Get(ESpriteFrame::Idle).First();
			}
		}
	}

	void CPlayer::MovementState_Running()
	{
		const glm::vec2 LinearVelocity = Body->GetLinearVelocity();
		const std::uint16_t FrameIndex = CRenderer::GetFrameIndex();
		const bool MovingByInput = (LastDirForce != 0.0f);

		if (std::abs(LinearVelocity.x) > VELOCITY_THRESHOLD_X) {
			SpriteFrame.Next = SpriteSheet->Get(ESpriteFrame::Walk).GetFrame(FrameIndex);
		} else if (!MovingByInput && std::abs(LinearVelocity.x) < VELOCITY_THRESHOLD_X) {
			SetMovementState(EMovementState::Idle);
			SpriteFrame.Next = SpriteSheet->Get(ESpriteFrame::Idle).First();
		}
	}

	void CPlayer::MovementState_Airborne()
	{
		const glm::vec2 LinearVelocity = Body->GetLinearVelocity();

		if (LinearVelocity.y > VELOCITY_THRESHOLD_Y) {
			SpriteFrame.Next = SpriteSheet->Get(ESpriteFrame::JumpAscend).First();
		} else if (LinearVelocity.y < -VELOCITY_THRESHOLD_Y) {
			SpriteFrame.Next = SpriteSheet->Get(ESpriteFrame::JumpDescend).First();
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
		TransformComp.Scale.x = BodySize.x * SpriteScale.x;
		TransformComp.Scale.y = BodySize.y * SpriteScale.y;
		TransformComp.Translation.x += SpriteOffset.x;
		TransformComp.Translation.y += SpriteOffset.y;
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
				if (Data.State == EKeyState::Pressed) {
					ThrowProjectile();
				}
				break;

			case EKey::F:
				break;

			case EKey::G:
				break;

			case EKey::D1:
				if (Data.State == EKeyState::Pressed) {
					Inventory.Select(0);
				}
				break;

			case EKey::D2:
				if (Data.State == EKeyState::Pressed) {
					Inventory.Select(1);
				}
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
					std::shared_ptr<IWeapon> Selected = Inventory.GetSelectedWeapon();
					if (!Selected) {
						break;
					}

					glm::vec2 TargetPos{0.0f, 0.0f};
					auto& GameInstance = CGameInstance::Get();
					if (CCamera* Camera = GameInstance.GetActiveCamera()) {
						const glm::vec2 World = GameInstance.GetMouseInWorldSpace(*Camera);
						if (Math::IsValid(World)) {
							TargetPos = World;
						}
					}

					if (Selected->GetWeaponType() == EWeaponType::Rifle) {
						CRifle& Rifle = static_cast<CRifle&>(*Selected);
						if (!Rifle.IsEnabled()) {
							break;
						}
					}

					LK_TRACE_TAG("Player", "PrimaryAction: {}", Enum::ToString(Selected->GetWeaponType()));
					Selected->PrimaryAction(TargetPos);
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
