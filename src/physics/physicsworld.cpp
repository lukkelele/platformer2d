#include "physicsworld.h"

#include "core/math/math.h"
#include "game/player.h"

namespace platformer2d {

	namespace
	{
		bool bInitialized = false;
		bool bPaused = false;
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

	void CPhysicsWorld::Shutdown()
	{
		b2DestroyWorld(WorldID);
	}

	void CPhysicsWorld::Update(const float DeltaTime)
	{
		if (!bPaused)
		{
			b2World_Step(WorldID, DeltaTime, Substep);
		}

		if (DebugDraw)
		{
			b2World_Draw(WorldID, DebugDraw.get());
		}
	}

	void CPhysicsWorld::Pause()
	{
		LK_DEBUG_TAG("PhysicsWorld", "Pause");
		bPaused = true;
	}

	void CPhysicsWorld::Unpause()
	{
		LK_DEBUG_TAG("PhysicsWorld", "Unpause");
		bPaused = false;
	}

	b2BodyId CPhysicsWorld::CreateBody(const b2BodyDef& BodyDef)
	{
		LK_ASSERT(bInitialized);
		return b2CreateBody(WorldID, &BodyDef);
	}

	void CPhysicsWorld::Destroy(CBody& Body)
	{
		LK_DEBUG_TAG("PhysicsWorld", "Delete: {}", Body.ID.index1);
		b2DestroyBody(Body.ID);
	}

	glm::vec2 CPhysicsWorld::GetGravity()
	{
		return Math::Convert<glm::vec2>(b2World_GetGravity(WorldID));
	}

	void CPhysicsWorld::SetGravity(const glm::vec2& Gravity)
	{
		b2World_SetGravity(WorldID, Math::Convert(Gravity));
	}

	void CPhysicsWorld::InitDebugDraw(b2DebugDraw& DebugDrawRef)
	{
		LK_ASSERT(bInitialized);
		DebugDraw = std::make_unique<b2DebugDraw>(DebugDrawRef);
	}

	bool CPhysicsWorld::PreSolve(b2ShapeId ShapeA, b2ShapeId ShapeB, b2Vec2 Point, b2Vec2 Normal, void* Ctx)
	{
		CPlayer& Player = *static_cast<CPlayer*>(Ctx);
		const b2ShapeId PlayerShapeID = Player.GetBody().ShapeID;

		LK_ASSERT(b2Shape_IsValid(ShapeA));
		LK_ASSERT(b2Shape_IsValid(ShapeB));
		float Sign = 0.0f;
		if (B2_ID_EQUALS(ShapeA, PlayerShapeID))
		{
			Sign = -1.0f;
		}
		else if (B2_ID_EQUALS(ShapeB, PlayerShapeID))
		{
			Sign = 1.0f;
		}
		else
		{
			/* Not colliding with the player, enable contact. */
			return true;
		}

		if ((Sign * Normal.y) > 0.95f)
		{
			return true;
		}

		/* Normal points down, disable contact. */
		return false;
	}


}