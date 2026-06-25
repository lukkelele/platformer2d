#include "enemyarchetype.h"

#include <array>
#include <utility>

namespace platformer2d {
	static constexpr FEnemyArchetype NoneArchetype = {};

	static constexpr FEnemyArchetype Grunt = {
		.MaxHealth = 60.0f,
		.MoveSpeed = 1.0f,
		.JumpImpulse = 0.0f,
		.DetectRadius = 4.0f,
		.StopRadius = 0.60f,
		.GiveUpRadius = 6.0f,
		.AttackDamage = 8.0f,
		.AttackRange = 0.50f,
		.LedgeProbeForward = 0.12f,
		.LedgeProbeDepth = 0.50f,
		.Size = {0.23f, 0.26f},
		.SpriteScale = {1.90f, 1.66f},
		.bCanJump = false,
		.bRanged = false,
	};

	static constexpr FEnemyArchetype Jumper = {
		.MaxHealth = 80.0f,
		.MoveSpeed = 1.20f,
		.JumpImpulse = 3.4f,
		.DetectRadius = 5.0f,
		.StopRadius = 0.60f,
		.GiveUpRadius = 8.0f,
		.AttackDamage = 10.0f,
		.AttackRange = 0.50f,
		.LedgeProbeForward = 0.18f,
		.LedgeProbeDepth = 0.60f,
		.Size = {0.38f, 0.46f},
		.bCanJump = true,
		.bRanged = false,
	};

	static constexpr FEnemyArchetype RangedShooter = {
		.MaxHealth = 70.0f,
		.MoveSpeed = 1.00f,
		.JumpImpulse = 0.0f,
		.DetectRadius = 8.0f,
		.StopRadius = 4.0f,
		.GiveUpRadius = 12.0f,
		.AttackDamage = 12.0f,
		.AttackRange = 6.0f,
		.LedgeProbeForward = 0.12f,
		.LedgeProbeDepth = 0.50f,
		.Size = {0.32f, 0.45f},
		.bCanJump = false,
		.bRanged = true,
	};

	static constexpr FEnemyArchetype TargetDummy = {
		.MaxHealth = 1000.0f,
		.MoveSpeed = 0.0f,
		.JumpImpulse = 0.0f,
		.DetectRadius = 0.0f,
		.StopRadius = 0.0f,
		.GiveUpRadius = 0.0f,
		.AttackDamage = 0.0f,
		.AttackRange = 0.0f,
		.ContactKnockback = 0.0f,
		.ContactKnockbackUp = 0.0f,
		.LedgeProbeForward = 0.0f,
		.LedgeProbeDepth = 0.0f,
		.Size = {0.18f, 0.24f},
		.SpriteScale = {3.40f, 2.42f},
		.bCanJump = false,
		.bRanged = false,
		.bImmortal = true,
	};

	static constexpr std::array<FEnemyArchetype, std::to_underlying(EEnemyArchetype::COUNT)> Registry = {
		NoneArchetype,
		Grunt,
		Jumper,
		RangedShooter,
		TargetDummy,
	};

	const FEnemyArchetype& GetEnemyArchetype(const EEnemyArchetype Archetype)
	{
		return Registry.at(static_cast<std::size_t>(Archetype));
	}
}

