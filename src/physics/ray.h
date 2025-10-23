#pragma once

#include "core/core.h"
#include "core/math/math.h"
#include "core/math/aabb.h"

namespace platformer2d {

	class CRay
	{
	public:
		CRay() = default;
		CRay(const glm::vec3& InOrigin, const glm::vec3& InDirection);
		~CRay() = default;

		bool IntersectsAABB(const FAABB& AABB, float& T) const;
		bool IntersectsTriangle(const glm::vec3& A, const glm::vec3& B, const glm::vec3& C, float& T) const;

	public:
		glm::vec3 Origin;
		glm::vec3 Direction;
	};

}
