#pragma once

#include <glm/glm.hpp>

#include "core/assert.h"

namespace platformer2d {

	glm::vec3 ConvertScreenToWorld(const glm::vec3& Point, const glm::vec3& Center,
								   float Width, float Height, float Zoom);
	glm::vec3 ConvertWorldToScreen(const glm::vec3& Point, const glm::vec3& Center,
								   float Width, float Height, float Zoom);

}