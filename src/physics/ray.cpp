#include "ray.h"

namespace platformer2d::Physics {

	void CastRay(FRayCast& RayCast, const glm::vec2& Pos, const glm::mat4& ViewMat,
				 const glm::mat4& ProjMat, float const MousePosX, const float MousePosY)
	{
		const glm::vec4 MouseClipPos = { MousePosX, MousePosY, -1.0f, 1.0f };
		const glm::mat4 InverseProj = glm::inverse(ProjMat);
		const glm::vec4 Ray = InverseProj * MouseClipPos;
		const glm::mat3 InverseView = glm::inverse(glm::mat3(ViewMat));

		RayCast.Pos = glm::vec3(Pos, 0.0f);
		RayCast.Dir = InverseView * glm::vec3(Ray);
	}

	bool RaycastAABB(const FRayCast& RayCast, const glm::vec2& BoxMin, const glm::vec2& BoxMax, float& OutT)
	{
		const glm::vec2 Origin = glm::vec2(RayCast.Pos.x, RayCast.Pos.y);
		const glm::vec2 Dir = glm::vec2(RayCast.Dir.x, RayCast.Dir.y);
		if ((Dir.x == 0.0f) && (Dir.y == 0.0f))
		{
			return false;
		}

		const glm::vec2 InvDir = glm::vec2(1.0f / Dir.x, 1.0f / Dir.y);
		const glm::vec2 T1 = (BoxMin - Origin) * InvDir;
		const glm::vec2 T2 = (BoxMax - Origin) * InvDir;

		const float TMin = glm::max(glm::min(T1.x, T2.x), glm::min(T1.y, T2.y));
		const float TMax = glm::min(glm::max(T1.x, T2.x), glm::max(T1.y, T2.y));

		if (TMax < 0.0f)
		{
			return false;
		}
		if (TMin > TMax)
		{
			return false;
		}

		OutT = TMin;
		return true;
	}

}