#include "physicsworld.h"

namespace platformer2d {

	void CPhysicsWorld::Initialize(const glm::vec2& Gravity)
	{
		b2WorldDef WorldDef = b2DefaultWorldDef();
		WorldDef.gravity = b2Vec2(Gravity.x, Gravity.y);
		WorldID = b2CreateWorld(&WorldDef);
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
		return b2CreateBody(WorldID, &BodyDef);
	}

}