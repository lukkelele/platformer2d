#pragma once

#include <string>

#include <glm/glm.hpp>

#include "core/delegate.h"
#include "scene/actor.h"

namespace platformer2d {

	struct FPlayerData
	{
		uint64_t ID = 0;
	};

	class CPlayer : public CActor
	{
	public:
		LK_DECLARE_EVENT(FOnJumped, CPlayer, const FPlayerData&);
	public:
		CPlayer(const FActorSpecification& Specification);
		CPlayer(CPlayer&&) = default;
		CPlayer(const CPlayer&) = default;
		~CPlayer() = default;

		virtual void Tick(float DeltaTime) override;
		void Jump();

		float GetMovementSpeed() const;

		/**
		 * @brief Set the movement speed.
		 * The speed should be greater than 1.0f.
		 */
		void SetMovementSpeed(float NewSpeed);
		void SetMovementSpeedFactor(float SpeedFactor);

	public:
		FOnJumped OnJumped;
	private:
		FPlayerData Data{};

		float MovementSpeed = 0.00032f;
		inline static float MovementSpeedFactor = 10000.0f;
	};

}