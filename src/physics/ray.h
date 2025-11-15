#pragma once

#include "core/core.h"
#include "core/math/aabb.h"
#include "core/math/math.h"

namespace platformer2d {

	struct FRayCast
	{
		glm::vec3 Pos = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Dir = { 1.0f, 1.0f, 1.0f };
	};

	namespace Physics
	{
		void CastRay(FRayCast& RayCast, const glm::vec2& Pos, const glm::mat4& ViewMat,
					 const glm::mat4& ProjMat, float MousePosX, float MousePosY);
		bool RaycastAABB(const FRayCast& RayCast, const glm::vec2& BoxMin, const glm::vec2& BoxMax, float& OutT);
	}

}
