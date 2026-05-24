#pragma once

#include "core/core.h"
#include "core/enum.h"

namespace platformer2d {

	enum class EWeaponType
	{
		Rifle,
		Melee,
		COUNT
	};
	LK_ENUM(EWeaponType);

	struct FRifleSpecification
	{
		std::uint16_t MagazineSize = 30;
	};

	struct FMeleeSpecification
	{
		float Damage = 18.0f;
		float Reach = 0.140f;
		float HitboxHeight = 0.100f;
		std::chrono::milliseconds WindupDuration{60};
		std::chrono::milliseconds ActiveDuration{120};
		std::chrono::milliseconds RecoveryDuration{180};
	};
}

