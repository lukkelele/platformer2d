#pragma once

#include "core/core.h"

namespace platformer2d {

	enum class EItemType
	{
		None,
		Consumable,
		Weapon,
		COUNT
	};

	namespace Enum {
		inline const char* ToString(const EItemType Item)
		{
			const char* S = "";
#define _(EnumValue)                                 \
	case EItemType::EnumValue: S = #EnumValue; break
			switch (Item) {
				_(None);
				_(Consumable);
				_(Weapon);
				_(COUNT);
				default:
					LK_THROW_ENUM_ERR(Item);
					break;
			}
#undef _
			return S;
		}
	}

}
