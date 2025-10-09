#pragma once

#include <string>

#include <glm/glm.hpp>

#include "core/core.h"
#include "core/assert.h"

namespace platformer2d {

	class CPlayer
	{
	public:
		CPlayer(std::string_view InName = "Player");
		CPlayer(CPlayer&&) = default;
		CPlayer(const CPlayer&) = default;
		~CPlayer() = default;

		void Tick(float DeltaTime = 0.0f);
		const glm::vec2& GetPosition() const { return Pos; }
		void SetPosition(float X, float Y);
		void SetPosition(const glm::vec2& NewPos);

		float GetMovementSpeed() const;

		/**
		 * @brief Set the movement speed.
		 * The speed should be greater than 1.0f.
		 */
		void SetMovementSpeed(float NewSpeed);

	private:
		glm::vec2 Pos{ 0.0f, 0.0f };
		std::string Name{};

		float MovementSpeed = 0.00032f;
		static constexpr float MovementSpeedFactor = 10000.0f;
	};

}