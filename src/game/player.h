#pragma once

#include <string>

#include <glm/glm.hpp>

#include "core/delegate.h"
#include "scene/actor.h"

namespace platformer2d {

	struct FPlayerData
	{
		uint64_t ID = 0;
		bool bJumping = false;
	};

	class CPlayer : public CActor
	{
	public:
		LK_DECLARE_EVENT(FOnJumped, CPlayer, const FPlayerData&);
		LK_DECLARE_EVENT(FOnLanded, CPlayer, const FPlayerData&);
	public:
		CPlayer(const FActorSpecification& Spec = FActorSpecification());
		CPlayer(const FBodySpecification& BodySpec);
		CPlayer(CPlayer&&) = default;
		CPlayer(const CPlayer&) = default;
		~CPlayer() = default;

		virtual void Tick(float DeltaTime) override;
		void Jump();

		const FPlayerData& GetData() const { return Data; }
		float GetMovementSpeed() const;

		void SetMovementSpeed(float NewSpeed);
		void SetMovementSpeedFactor(float SpeedFactor);

		inline float GetJumpImpulse() const { return JumpImpulse; }
		void SetJumpImpulse(float Impulse);

		inline float GetDirectionForce() const { return DirForce; }
		void SetDirectionForce(float Force);

	private:
		void CheckJumpState();

	public:
		FOnJumped OnJumped;
		FOnLanded OnLanded;
	private:
		FPlayerData Data{};

		float MovementSpeed = 3.20f * MovementSpeedFactor;
		static inline float MovementSpeedFactor = 0.000010f;

		float JumpImpulse = 0.92f;
		float DirForce = 1.460f;
	};

}