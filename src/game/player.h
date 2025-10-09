#pragma once

#include <string>

#include <glm/glm.hpp>

#include "actor.h"

namespace platformer2d {

	class CPlayer : public CActor
	{
	public:
		CPlayer(std::string_view InName = "Player");
		CPlayer(CPlayer&&) = default;
		CPlayer(const CPlayer&) = default;
		~CPlayer() = default;

		void Tick(float DeltaTime = 0.0f);

		float GetMovementSpeed() const;

		/**
		 * @brief Set the movement speed.
		 * The speed should be greater than 1.0f.
		 */
		void SetMovementSpeed(float NewSpeed);

	private:
		std::string Name{};

		float MovementSpeed = 0.00032f;
		static constexpr float MovementSpeedFactor = 10000.0f;
	};

}