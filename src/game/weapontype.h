#pragma once

#include "core/core.h"
#include "core/enum.h"

namespace platformer2d {

	enum class EWeaponType
	{
		Rifle,
		COUNT
	};
	LK_ENUM(EWeaponType);

	struct FRifleSpecification
	{
		std::uint16_t MagazineSize = 30;
	};
}

