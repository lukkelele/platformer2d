#pragma once

#include "core/delegate.h"
#include "core/enum.h"
#include "core/timer.h"
#include "renderer/camera.h"
#include "renderer/sprite.h"
#include "scene/actor.h"
#include "inventory.h"
#include "rifle.h"
#include "spritereader.h"

namespace platformer2d {

	struct FPlayerData
	{
		std::uint64_t ID = 0;
		EMovementState MovementState = EMovementState::Idle;
		bool bJumping = false;
	};

	class CPlayer : public CActor
	{
	public:
		LK_DECLARE_EVENT(FOnJumped, CPlayer, const FPlayerData&);
		LK_DECLARE_EVENT(FOnLanded, CPlayer, const FPlayerData&);
		LK_DECLARE_EVENT(FOnDied, CPlayer, const FPlayerData&);

	public:
		CPlayer(const FActorSpecification&, const FBodySpecification& BodySpec);
		CPlayer(CPlayer&&) = default;
		CPlayer(const CPlayer&) = default;
		~CPlayer();

		void Tick(float DeltaTime) override;
		void OnDeath(EDeathReason Reason) override;
		EActorType GetActorType() const override { return EActorType::Player; }
		void Jump();

		[[nodiscard]] const FPlayerData& GetData() const { return Data; }
		[[nodiscard]] CInventory& GetInventory() { return Inventory; }
		[[nodiscard]] const CInventory& GetInventory() const { return Inventory; }
		[[nodiscard]] const CSprite* GetSprite() const override { return Sprite.get(); }

		[[nodiscard]] float GetJumpImpulse() const { return JumpImpulse; }
		void SetJumpImpulse(float Impulse);
		[[nodiscard]] float GetDirectionForce() const { return DirForce; }
		void SetDirectionForce(float Force);
		[[nodiscard]] float GetLastDirectionForce() const { return LastDirForce; }
		[[nodiscard]] EDirection GetLookDirection() const { return LookDir; }
		void SetAwake(bool Awake) const;

		void SetLookDirection(EDirection InDirection);

		[[nodiscard]] CCamera& GetCamera() { return *GetComponent<FCameraComponent>().Camera; }
		[[nodiscard]] const CCamera& GetCamera() const { return *GetComponent<FCameraComponent>().Camera; }
		[[nodiscard]] bool IsCameraLocked() const { return bCameraLock; }
		void SetCameraLock(bool Locked);

		[[nodiscard]] std::pair<FSpriteCoord, FSpriteCoord> GetCurrentAndNextSpriteFrame() const { return std::make_pair(SpriteFrame.Current, SpriteFrame.Next); }
		[[nodiscard]] const FSpriteSheet* GetSpriteSheet() const { return SpriteSheet; }
		void SetSpriteSheet(ETexture InTexture);

		[[nodiscard]] float GetSpriteScale() const { return SpriteScale; }
		void SetSpriteScale(const float InScale) { SpriteScale = InScale; }
		[[nodiscard]] const glm::vec2& GetSpriteOffset() const { return SpriteOffset; }
		void SetSpriteOffset(const glm::vec2& InOffset) { SpriteOffset = InOffset; }

		[[nodiscard]] bool HasRifle();
		[[nodiscard]] std::shared_ptr<CRifle> GetRifle();

		void ThrowProjectile();
		[[nodiscard]] bool CanThrowProjectile() const;
		void SetProjectileThrowSpeed(const float InSpeed) { ProjectileThrowSpeed = InSpeed; }
		[[nodiscard]] float GetProjectileThrowSpeed() const { return ProjectileThrowSpeed; }

		[[nodiscard]] bool IsInClimbZone() const { return bInClimbZone; }
		[[nodiscard]] float GetClimbSpeed() const { return ClimbSpeed; }
		void SetClimbZone(bool InZone, float InClimbSpeed = 0.0f);

		bool Serialize(YAML::Emitter& Out, EExtendableSerializer Extendable = EExtendableSerializer::Yes) const override;

		void OnWindowResized(std::uint16_t Width, std::uint16_t Height);
		void OnKey(const FKeyData& Data);
		void OnMouseButton(const FMouseButtonData& Data);
		void OnMouseScroll(EMouseScrollDirection Direction);

	private:
		void HandleInput();
		void OnInputReceived();
		bool InputReceivedLastTick() const { return (LastDirForce != 0.0f); }

		void UpdateMovementState();
		void MovementState_Idle();
		void MovementState_Running();
		void MovementState_Airborne();
		void SetMovementState(EMovementState State);

		void CheckCollisions();
		void SyncTransformComponent();
		void UpdateSprite();
		void ForceUpdateSprite();
		void SetSpriteTilePos(const FSpriteCoord& InCoord, bool ForceUpdate = false);

	public:
		FOnJumped OnJumped;
		FOnLanded OnLanded;
		FOnDied OnDied;

	private:
		FPlayerData Data{};
		CTimer Timer;
		bool bCameraLock = true;
		CInventory Inventory;

		EDirection LookDir = EDirection::Right;
		float JumpImpulse = 3.440f;
		float DirForce = 5.630f;
		float LastDirForce = 0.0f;

		bool bJustLanded = false;
		bool bMovementInputLastTick = false;
		std::chrono::steady_clock::time_point LastInputTime;
		bool bWantToClimb = false;
		bool bInClimbZone = false;
		float ClimbSpeed = 0.0f;

		bool bShouldUpdateSprite = false;

		std::unique_ptr<CSprite> Sprite = nullptr;
		const FSpriteSheet* SpriteSheet = nullptr;
		FSpriteFrame SpriteFrame;

		float SpriteScale = 1.0f;
		glm::vec2 SpriteOffset{0.0f, 0.0f};

		std::chrono::steady_clock::time_point NextProjectileTime{};
		std::chrono::milliseconds ProjectileCooldown = 650ms;
		std::chrono::milliseconds ProjectileExpireTimeout = 3000ms;
		float ProjectileThrowSpeed = 3.5f;
		float ProjectileRadius = 0.040f;
		float ProjectileRestitution = 0.42f;
		float ProjectileDamage = 12.0f;

		LK_CLASS();
	};
}

