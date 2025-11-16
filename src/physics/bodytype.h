#pragma once

#include "core/core.h"

namespace platformer2d {

	enum class EBodyType
	{
		Static,
		Dynamic,
		Kinematic,
	};

	namespace Enum
	{
		inline const char* ToString(const EBodyType Type)
		{
			const char* S = "";
		#define _(EnumValue) case EBodyType::EnumValue: S = #EnumValue; break
			switch (Type)
			{
				_(Static);
				_(Dynamic);
				_(Kinematic);
				default:
					LK_THROW_ENUM_ERR(Type);
					break;
			}
		#undef _
			return S;
		}
	}

}