#pragma once

#include <glm/glm.hpp>

#include "core/assert.h"

namespace platformer2d {

	struct FAABB
	{
		glm::vec2 Min{};
		glm::vec2 Max{};
	};

}