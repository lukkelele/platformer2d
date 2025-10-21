#include "physicsworld.h"

namespace platformer2d {

	namespace
	{
		bool bInitialized;
	}

	void CPhysicsWorld::Initialize(const glm::vec2& Gravity)
	{
		LK_VERIFY(!bInitialized, "Initialize called multiple times");
		b2WorldDef WorldDef = b2DefaultWorldDef();
		WorldDef.gravity = b2Vec2(Gravity.x, Gravity.y);
		WorldID = b2CreateWorld(&WorldDef);
		LK_DEBUG_TAG("PhysicsWorld", "WorldID={} Substep={}", WorldID.index1, Substep);

		bInitialized = true;
	}

	void CPhysicsWorld::Destroy()
	{
		b2DestroyWorld(WorldID);
	}

	void CPhysicsWorld::Update(const float DeltaTime)
	{
		b2World_Step(WorldID, DeltaTime, Substep);
	}

	b2BodyId CPhysicsWorld::CreateBody(const b2BodyDef& BodyDef)
	{
		LK_ASSERT(bInitialized);
		return b2CreateBody(WorldID, &BodyDef);
	}

}