#include "ray.h"

namespace platformer2d {

	CRay::CRay(const glm::vec3& InOrigin, const glm::vec3& InDirection)
		: Origin(InOrigin)
		, Direction(InDirection)
	{
	}

	bool CRay::IntersectsAABB(const FAABB& AABB, float& T) const
	{
		/* The direction vector is inverted to calculate the intersection points. */
		glm::vec3 DirFraction{};
		DirFraction.x = (1.0f / Direction.x);
		DirFraction.y = (1.0f / Direction.y);
		DirFraction.z = (1.0f / Direction.z);

		const glm::vec3& LeftBottom = AABB.Min;
		const glm::vec3& RightTop = AABB.Max;

		const float T1 = (LeftBottom.x - Origin.x) * DirFraction.x;
		const float T2 = (RightTop.x - Origin.x) * DirFraction.x;

		const float T3 = (LeftBottom.y - Origin.y) * DirFraction.y;
		const float T4 = (RightTop.y - Origin.y) * DirFraction.y;

		const float T5 = (LeftBottom.z - Origin.z) * DirFraction.z;
		const float T6 = (RightTop.z - Origin.z) * DirFraction.z;

		/* Point where the ray first enters the AABB. */
		const float TMin = glm::max(
			glm::max(glm::min(T1, T2), glm::min(T3, T4)), 
			glm::min(T5, T6)
		);

		/* Point where the ray exits the AABB. */
		const float TMax = glm::min(
			glm::min(glm::max(T1, T2), glm::max(T3, T4)), 
			glm::max(T5, T6)
		);

		/* Ray is intersecting with the AABB behind us. */
		if (TMax < 0)
		{
			T = TMax;
			return false;
		}

		/* Ray does not intersect the AABB. */
		if (TMin > TMax)
		{
			T = TMax;
			return false;
		}

		/* When T == 0, T is at the ray's origin. */
		T = TMin;
		return true;
	}

	bool CRay::IntersectsTriangle(const glm::vec3& A, const glm::vec3& B, const glm::vec3& C, float& T) const
	{
		const glm::vec3 E1 = B - A;
		const glm::vec3 E2 = C - A;

		const glm::vec3 N = glm::cross(E1, E2);
		const float Det = -glm::dot(Direction, N);

		const float InvDet = 1.0f / Det;

		const glm::vec3 AO = Origin - glm::vec3(A);
		const glm::vec3 DAO = glm::cross(AO, Direction);
		const float U = glm::dot(E2, DAO) * InvDet;
		const float V = -glm::dot(E1, DAO) * InvDet;

		T = glm::dot(AO, N) * InvDet;

		return ((Det >= 1e-6f) 
				&& (T >= 0.0f) 
				&& (U >= 0.0f) 
				&& (V >= 0.0f) 
				&& ((U + V) <= 1.0f));
	}

}