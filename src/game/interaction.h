#pragma once

#include "core/core.h"

namespace platformer2d {

	enum class EInteraction
	{
		None,
		Damage,
		Pickup,
		COUNT
	};

	namespace Enum
	{
		inline const char* ToString(const EInteraction Interaction)
		{
			const char* S = "";
		#define _(EnumValue) case EInteraction::EnumValue: S = #EnumValue; break
			switch (Interaction)
			{
				_(None);
				_(Damage);
				_(Pickup);
				_(COUNT);
				default:
					LK_THROW_ENUM_ERR(Interaction);
					break;
			}
		#undef _
			return S;
		}
	}

}
