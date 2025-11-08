#pragma once

#include "core/delegate.h"
#include "core/timer.h"
#include "renderer/camera.h"
#include "renderer/sprite.h"
#include "scene/actor.h"

namespace platformer2d {

	enum class EMovementState
	{
		Idle,
		Running,
		Airborne,
	};

	enum class EDirection
	{
		Up,
		Down,
		Left,
		Right
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
		CPlayer(const FActorSpecification& Spec = FActorSpecification(), ETexture InTexture = ETexture::Player);
		CPlayer(const FBodySpecification& BodySpec, ETexture InTexture = ETexture::Player);
		CPlayer(CPlayer&&) = default;
		CPlayer(const CPlayer&) = default;
		~CPlayer() = default;

		virtual void Tick(float DeltaTime) override;
		void Jump();

		inline const FPlayerData& GetData() const { return Data; }
		inline const CSprite& GetSprite() const { return *Sprite; }

		float GetJumpImpulse() const { return JumpImpulse; }
		void SetJumpImpulse(float Impulse);
		float GetDirectionForce() const { return DirForce; }
		void SetDirectionForce(float Force);
		float GetLastDirectionForce() const { return LastDirForce; }
		EDirection GetLookDirection() const { return LookDir; }

		inline CCamera& GetCamera() { return *Camera; }
		inline const CCamera& GetCamera() const { return *Camera; }
		void SetCameraLock(bool Locked);

		/**
		 * @brief Get scaled size.
		 */
		glm::vec2 GetSize() const;

	private:
		void HandleInput();

		void UpdateMovementState();
		void MovementState_Idle();
		void MovementState_Running();
		void MovementState_Airborne();
		void SetMovementState(EMovementState State);

		void CheckCollisions();
		void SyncTransformComponent();
		void UpdateSprite();
		void SetSpriteTilePos(uint16_t X);

		void OnWindowResized(uint16_t Width, uint16_t Height);
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
		float JumpImpulse = 2.890f;
		float DirForce = 5.630f;
		float LastDirForce = 0.0f;

		bool bJustLanded = false;
		bool bMovementInputLastTick = false;
		bool bShouldUpdateSprite = false;

		std::unique_ptr<CSprite> Sprite = nullptr;
		FSpriteAnimation WalkAnim;
		uint16_t CurrentSpriteFrame = 0;
		uint16_t NextSpriteFrame = 0;
	};

	namespace Enum
	{
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

		inline const char* ToString(const EDirection Direction)
		{
			const char* S = "";
		#define _(EnumValue) case EDirection::EnumValue: S = #EnumValue; break
			switch (Direction)
			{
				_(Up);
				_(Down);
				_(Left);
				_(Right);
				default:
					LK_THROW_ENUM_ERR(Direction);
					break;
			}
		#undef _
			return S;
		}
	}

}