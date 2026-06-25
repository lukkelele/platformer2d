#pragma once

#include <cstdint>

#include <glm/glm.hpp>

#include "core/core.h"
#include "core/enum.h"

namespace platformer2d {

	enum class EEnemyArchetype : std::uint8_t
	{
		None,
		Grunt,
		Jumper,
		RangedShooter,
		TargetDummy,
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
		float ContactKnockback = 3.0f;
		float ContactKnockbackUp = 2.5f;
		float HitCooldown = 0.60f;
		float LedgeProbeForward = 0.12f;
		float LedgeProbeDepth = 0.50f;
		glm::vec2 Size = {0.30f, 0.45f};
		glm::vec2 SpriteScale = {1.0f, 1.0f};
		bool bCanJump = false;
		bool bRanged = false;
		bool bImmortal = false;
	};

	[[nodiscard]] const FEnemyArchetype& GetEnemyArchetype(EEnemyArchetype Archetype);
}

