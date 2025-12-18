#pragma once

#include "core/core.h"

namespace platformer2d {

	enum class EWeaponType
	{
		Rifle,
		COUNT
	};

	struct FRifleSpecification
	{
		uint16_t MagazineSize = 30;
	};

	namespace Enum {
		inline const char* ToString(const EWeaponType Type)
		{
			const char* S = "";
		#define _(EnumValue) case EWeaponType::EnumValue: S = #EnumValue; break
			switch (Type) {
				_(Rifle);
				_(COUNT);
				default:
					LK_THROW_ENUM_ERR(Type);
					break;
			}
		#undef _
			return S;
		}
	}

}
