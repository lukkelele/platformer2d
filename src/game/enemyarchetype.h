#pragma once

#include <cstdint>

#include <glm/glm.hpp>

#include "core/core.h"
#include "core/enum.h"

namespace platformer2d {

	enum class EEnemyArchetype : std::uint8_t
	{
		Grunt,
		Jumper,
		RangedShooter,
		COUNT
	};
	LK_ENUM(EEnemyArchetype);

	struct FEnemyArchetype
	{
		float MaxHealth = 100.0f;
		float MoveSpeed = 1.20f;
		float JumpImpulse = 3.0f;
		float DetectRadius = 4.0f;
		float StopRadius = 0.60f;
		float GiveUpRadius = 6.0f;
		float AttackDamage = 10.0f;
		float AttackRange = 0.50f;
		glm::vec2 Size = {0.40f, 0.60f};
		bool bCanJump = false;
		bool bRanged = false;
	};

	[[nodiscard]] const FEnemyArchetype& GetArchetype(EEnemyArchetype Archetype);
}

