#include "enemyarchetype.h"

#include <array>
#include <utility>

namespace platformer2d {

	static constexpr FEnemyArchetype Grunt = {
		.MaxHealth = 60.0f,
		.MoveSpeed = 1.20f,
		.JumpImpulse = 0.0f,
		.DetectRadius = 4.0f,
		.StopRadius = 0.60f,
		.GiveUpRadius = 6.0f,
		.AttackDamage = 8.0f,
		.AttackRange = 0.50f,
		.Size = {0.40f, 0.60f},
		.bCanJump = false,
		.bRanged = false,
	};

	static constexpr FEnemyArchetype Jumper = {
		.MaxHealth = 80.0f,
		.MoveSpeed = 1.80f,
		.JumpImpulse = 3.4f,
		.DetectRadius = 5.0f,
		.StopRadius = 0.60f,
		.GiveUpRadius = 8.0f,
		.AttackDamage = 10.0f,
		.AttackRange = 0.50f,
		.Size = {0.40f, 0.60f},
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
		.Size = {0.40f, 0.60f},
		.bCanJump = false,
		.bRanged = true,
	};

	static constexpr std::array<FEnemyArchetype, std::to_underlying(EEnemyArchetype::COUNT)> Registry = {
		Grunt,
		Jumper,
		RangedShooter,
	};

	const FEnemyArchetype& GetArchetype(const EEnemyArchetype Archetype)
	{
		return Registry.at(std::to_underlying(Archetype));
	}
}

