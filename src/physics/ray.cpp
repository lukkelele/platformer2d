#include "ray.h"

#include <box2d/box2d.h>

#include "collisionfilter.h"
#include "physicsworld.h"

namespace platformer2d::Physics {

	void CastRay(FRayCast& RayCast, const glm::vec2& Pos, const glm::mat4& ViewMat,
		const glm::mat4& ProjMat, float const MousePosX, const float MousePosY)
	{
		const glm::vec4 MouseClipPos = {MousePosX, MousePosY, -1.0f, 1.0f};
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
		if ((Dir.x == 0.0f) && (Dir.y == 0.0f)) {
			return false;
		}

		const glm::vec2 InvDir = glm::vec2(1.0f / Dir.x, 1.0f / Dir.y);
		const glm::vec2 T1 = (BoxMin - Origin) * InvDir;
		const glm::vec2 T2 = (BoxMax - Origin) * InvDir;

		const float TMin = glm::max(glm::min(T1.x, T2.x), glm::min(T1.y, T2.y));
		const float TMax = glm::min(glm::max(T1.x, T2.x), glm::max(T1.y, T2.y));
		if (TMax < 0.0f) {
			return false;
		}
		if (TMin > TMax) {
			return false;
		}

		OutT = TMin;
		return true;
	}

	bool HasLineOfSight(const glm::vec2& From, const glm::vec2& To, const CActor* const Target)
	{
		if (!Target || !CPhysicsWorld::IsValid()) {
			return false;
		}

		const b2Vec2 Origin = {From.x, From.y};
		const b2Vec2 Translation = {To.x - From.x, To.y - From.y};
		if ((Translation.x == 0.0f) && (Translation.y == 0.0f)) {
			return true;
		}

		const b2QueryFilter Filter = {.categoryBits = ~0ULL, .maskBits = COLLISION_QUERY_SIGHT};
		const b2RayResult Result = b2World_CastRayClosest(CPhysicsWorld::GetID(), Origin, Translation, Filter);
		if (!Result.hit) {
			return true;
		}

		const CActor* HitActor = static_cast<const CActor*>(b2Shape_GetUserData(Result.shapeId));
		return (HitActor == Target);
	}

	bool ProbeGround(const glm::vec2& From, const float Depth)
	{
		if (!CPhysicsWorld::IsValid()) {
			return false;
		}

		const b2Vec2 Origin = {From.x, From.y};
		const b2Vec2 Translation = {0.0f, -Depth};
		const b2QueryFilter Filter = {.categoryBits = ~0ULL, .maskBits = COLLISION_QUERY_GROUND};
		const b2RayResult Result = b2World_CastRayClosest(CPhysicsWorld::GetID(), Origin, Translation, Filter);
		return Result.hit;
	}
}

