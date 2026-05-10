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
	LK_ENUM(EBodyType);

}

