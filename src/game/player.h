#pragma once

#include "core/delegate.h"
#include "core/timer.h"
#include "renderer/camera.h"
#include "renderer/sprite.h"
#include "scene/actor.h"
#include "rifle.h"

namespace platformer2d {

	enum class EMovementState
	{
		Idle,
		Running,
		Airborne,
	};

	struct FPlayerData
	{
		uint64_t ID = 0;
		EMovementState MovementState = EMovementState::Idle;
		bool bJumping = false;
	};

	class CPlayer : public CActor
	{
	public:
		LK_DECLARE_EVENT(FOnJumped, CPlayer, const FPlayerData&);
		LK_DECLARE_EVENT(FOnLanded, CPlayer, const FPlayerData&);
	public:
		CPlayer(const FActorSpecification&, const FBodySpecification& BodySpec);
		CPlayer(CPlayer&&) = default;
		CPlayer(const CPlayer&) = default;
		~CPlayer();

		virtual void Tick(float DeltaTime) override;
		virtual EActorType GetActorType() const override { return EActorType::Player; }
		void Jump();

		inline const FPlayerData& GetData() const { return Data; }
		inline const CSprite& GetSprite() const { return *Sprite; }

		float GetJumpImpulse() const { return JumpImpulse; }
		void SetJumpImpulse(float Impulse);
		float GetDirectionForce() const { return DirForce; }
		void SetDirectionForce(float Force);
		float GetLastDirectionForce() const { return LastDirForce; }
		EDirection GetLookDirection() const { return LookDir; }
		void SetAwake(bool Awake) const;

		void SetLookDirection(EDirection InDirection);

		inline CCamera& GetCamera() { return *Camera; }
		inline const CCamera& GetCamera() const { return *Camera; }
		bool IsCameraLocked() const { return bCameraLock; }
		void SetCameraLock(bool Locked);

		std::pair<uint16_t, uint16_t> GetCurrentAndNextSpriteFrame() const { return std::make_pair(CurrentSpriteFrame, NextSpriteFrame); }

		virtual bool Serialize(YAML::Emitter& Out) const override;

		std::shared_ptr<CRifle> GetRifle() { return Rifle; }

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
		void SetSpriteTilePos(uint16_t X, bool ForceUpdate = false);

		void OnWindowResized(uint16_t Width, uint16_t Height);
		void OnKeyPressed(const FKeyData& Data);
		void OnMouseButtonPressed(const FMouseButtonData& Data);
		void OnMouseScrolled(EMouseScrollDirection Direction);

	public:
		FOnJumped OnJumped;
		FOnLanded OnLanded;
	private:
		FPlayerData Data{};
		CTimer Timer;
		std::unique_ptr<CCamera> Camera = nullptr;
		bool bCameraLock = true;

		EDirection LookDir = EDirection::Right;
		float JumpImpulse = 3.440f;
		float DirForce = 5.630f;
		float LastDirForce = 0.0f;

		bool bJustLanded = false;
		bool bMovementInputLastTick = false;
		std::chrono::steady_clock::time_point LastInputTime;
		bool bWantToClimb = false;

		bool bShouldUpdateSprite = false;

		std::unique_ptr<CSprite> Sprite = nullptr;
		FSpriteAnimation WalkAnim;
		uint16_t CurrentSpriteFrame = 0;
		uint16_t NextSpriteFrame = 0;

		std::shared_ptr<CRifle> Rifle = nullptr;
	};

	namespace Enum {
		inline const char* ToString(const EMovementState State)
		{
			const char* S = "";
		#define _(EnumValue) case EMovementState::EnumValue: S = #EnumValue; break
			switch (State)
			{
				_(Idle);
				_(Running);
				_(Airborne);
				default:
					LK_THROW_ENUM_ERR(State);
					break;
			}
		#undef _
			return S;
		}
	}

}