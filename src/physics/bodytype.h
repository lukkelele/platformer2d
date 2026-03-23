#pragma once

#include "core/core.h"

namespace platformer2d {

	enum class EBodyType : std::uint8_t
	{
		Static,
		Dynamic,
		Kinematic,
		COUNT
	};

	namespace Enum {
		inline const char* ToString(const EBodyType Type)
		{
			const char* S = "";
#define _(EnumValue)                                 \
	case EBodyType::EnumValue: S = #EnumValue; break
			switch (Type) {
				_(Static);
				_(Dynamic);
				_(Kinematic);
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
