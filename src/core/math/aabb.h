#pragma once

#include <glm/glm.hpp>

#include "core/assert.h"

namespace platformer2d {

	struct FAABB
	{
		glm::vec3 Min{};
		glm::vec3 Max{};

		FAABB(const glm::vec3& InMin, const glm::vec3& InMax)
			: Min(InMin)
			, Max(InMax)
		{
		}
	};

}