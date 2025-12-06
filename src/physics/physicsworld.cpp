#include "physicsworld.h"

#include "core/math/math.h"
#include "game/player.h"
#include "renderer/renderer.h"

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
		LK_TRACE_TAG("PhysicsWorld", "WorldID={} Substep={}", WorldID.index1, Substep);

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
			HandleSensorEvents();
			HandleContactEvents();
		}

		if (DebugDraw)
		{
			CRenderer::Submit([&]()
			{
				b2World_Draw(WorldID, DebugDraw.get());
			});
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

	void CPhysicsWorld::SetPreSolve(const TPreSolveFunc InPreSolve, void* Context)
	{
		LK_ASSERT(InPreSolve != nullptr);
		b2World_SetPreSolveCallback(WorldID, InPreSolve, Context);
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
		LK_ASSERT(b2Shape_IsValid(ShapeA) && b2Shape_IsValid(ShapeB));
		CPlayer& Player = *static_cast<CPlayer*>(Ctx);
		const b2ShapeId PlayerShapeID = Player.GetBody()->GetShapeID();

		const bool InvolvesPlayer = B2_ID_EQUALS(ShapeA, PlayerShapeID) || B2_ID_EQUALS(ShapeB, PlayerShapeID);
		if (!InvolvesPlayer)
		{
			return true; /* Enable normal contacts. */
		}

		const CActor* ActorA = static_cast<CActor*>(b2Shape_GetUserData(ShapeA));
		const CActor* ActorB = static_cast<CActor*>(b2Shape_GetUserData(ShapeB));

		/* Make normal point from platform to player. */
		if (B2_ID_EQUALS(ShapeA, PlayerShapeID))
		{
			Normal.x = -Normal.x;
			Normal.y = -Normal.y;
		}

		const b2Vec2 Up = { 0.0f, 1.0f };
		const float UpDot = Normal.x * Up.x + Normal.y * Up.y;
		if (UpDot <= 0.0f)
		{
			/* Side/ceiling/backface -> behave as a solid. */
			return true;
		}

		const b2BodyId PlayerBody = Player.GetBody()->GetID();
		const b2Vec2 V = b2Body_GetLinearVelocity(PlayerBody);
		const float Vn = V.x * Normal.x + V.y * Normal.y;
		if (Vn > 0.0f)
		{
			/* Moving along the normal (from below toward the platform) -> ignore contact. */
			return false;
		}

		return true;
	}

	void CPhysicsWorld::HandleSensorEvents()
	{
		b2SensorEvents Events = b2World_GetSensorEvents(WorldID);

		/* Begin */
		for (int Idx = 0; Idx < Events.beginCount; Idx++)
		{
			b2SensorBeginTouchEvent* EventRef = Events.beginEvents + Idx;
			b2ShapeId SensorShape = EventRef->sensorShapeId;
			b2ShapeId VisitorShape = EventRef->visitorShapeId;
			if (!b2Shape_IsValid(SensorShape) || !b2Shape_IsValid(VisitorShape))
			{
				continue;
			}

			CActor* Sensor = static_cast<CActor*>(b2Shape_GetUserData(SensorShape));
			CActor* Visitor = static_cast<CActor*>(b2Shape_GetUserData(VisitorShape));
			CSensorBeginEvent Event(Sensor, Visitor);
			OnSensorBeginEvent.Broadcast(Event);
		}

		/* End */
		for (int Idx = 0; Idx < Events.endCount; Idx++)
		{
			b2SensorEndTouchEvent* EventRef = Events.endEvents + Idx;
			b2ShapeId SensorShape = EventRef->sensorShapeId;
			b2ShapeId VisitorShape = EventRef->visitorShapeId;
			if (!b2Shape_IsValid(SensorShape) || !b2Shape_IsValid(VisitorShape))
			{
				continue;
			}

			CActor* Sensor = static_cast<CActor*>(b2Shape_GetUserData(SensorShape));
			CActor* Visitor = static_cast<CActor*>(b2Shape_GetUserData(VisitorShape));
			CSensorEndEvent Event(Sensor, Visitor);
			OnSensorEndEvent.Broadcast(Event);
		}
	}

	void CPhysicsWorld::HandleContactEvents()
	{
		b2ContactEvents Events = b2World_GetContactEvents(WorldID);

		/* Begin */
		for (int Idx = 0; Idx < Events.beginCount; Idx++)
		{
			b2ContactBeginTouchEvent* EventRef = Events.beginEvents + Idx;
			b2ShapeId ShapeA = EventRef->shapeIdA;
			b2ShapeId ShapeB = EventRef->shapeIdB;
			if (!b2Shape_IsValid(ShapeA) || !b2Shape_IsValid(ShapeB))
			{
				continue;
			}

			CActor* A = static_cast<CActor*>(b2Shape_GetUserData(ShapeA));
			CActor* B = static_cast<CActor*>(b2Shape_GetUserData(ShapeB));
			CContactBeginEvent Event(A, B);
			if (!A || !B)
			{
				LK_ERROR("A={}  B={}", A ? "OK" : "NULL", B ? "OK" : "NULL");
			}
			OnContactBeginEvent.Broadcast(Event);
		}

		/* End */
		for (int Idx = 0; Idx < Events.endCount; Idx++)
		{
			b2ContactEndTouchEvent* EventRef = Events.endEvents + Idx;
			b2ShapeId ShapeA = EventRef->shapeIdA;
			b2ShapeId ShapeB = EventRef->shapeIdB;
			if (!b2Shape_IsValid(ShapeA) || !b2Shape_IsValid(ShapeB))
			{
				continue;
			}

			CActor* A = static_cast<CActor*>(b2Shape_GetUserData(ShapeA));
			CActor* B = static_cast<CActor*>(b2Shape_GetUserData(ShapeB));
			if (!A || !B)
			{
				LK_ERROR("A={}  B={}", A ? "OK" : "NULL", B ? "OK" : "NULL");
			}
			CContactEndEvent Event(A, B);
			OnContactEndEvent.Broadcast(Event);
		}
	}

}