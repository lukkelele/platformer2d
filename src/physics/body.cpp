#include "body.h"

#include "core/math/math.h"
#include "physicsworld.h"

namespace platformer2d {

	CBody::CBody(const FBodySpecification& Spec)
	{
		ShapeType = DetermineShapeType(Spec.Shape);

		b2BodyDef BodyDef = b2DefaultBodyDef();
		SetBodyDef(BodyDef, Spec);

		b2ShapeDef ShapeDef = b2DefaultShapeDef();
		if (Spec.Flags & EBodyFlag_PreSolveEvents)
		{
			ShapeDef.enablePreSolveEvents = true;
		}
		if (Spec.Flags & EBodyFlag_ContactEvents)
		{
			ShapeDef.enableContactEvents = true;
		}
		if (Spec.Flags & EBodyFlag_SensorEvents)
		{
			ShapeDef.enableSensorEvents = true;
		}

		ShapeDef.material.friction = Spec.Friction;
		ShapeDef.isSensor = Spec.bSensor;

		ID = CPhysicsWorld::CreateBody(BodyDef);
		LK_DEBUG_TAG("Body", "New body: {}", static_cast<int>(BodyDef.type));
		Shape = Spec.Shape;

		if (std::holds_alternative<FPolygon>(Spec.Shape))
		{
			LK_ASSERT(ShapeType == EShape::Polygon);
			const FPolygon& ShapeRef = std::get<FPolygon>(Spec.Shape);
			LK_ASSERT((ShapeRef.Size.x > 0.0f) && (ShapeRef.Size.y > 0.0f), "Invalid size");
			b2Polygon Polygon = b2MakeBox(ShapeRef.Size.x * 0.50f, ShapeRef.Size.y * 0.50f);
			ShapeID = b2CreatePolygonShape(ID, &ShapeDef, &Polygon);
		}
		else if (std::holds_alternative<FLine>(Spec.Shape))
		{
			LK_ASSERT(ShapeType == EShape::Line);
			LK_ASSERT(false);
		}
		else if (std::holds_alternative<FCapsule>(Spec.Shape))
		{
			LK_ASSERT(ShapeType == EShape::Capsule);
			const FCapsule& ShapeRef = std::get<FCapsule>(Spec.Shape);
			const b2Capsule Capsule = {
				{ ShapeRef.P0.x, ShapeRef.P0.y },
				{ ShapeRef.P1.x, ShapeRef.P1.y },
				ShapeRef.Radius
			};
			ShapeID = b2CreateCapsuleShape(ID, &ShapeDef, &Capsule);
		}

		SetMass(1.0f);
	}

	void CBody::Tick(const float InDeltaTime)
	{
		DeltaTime = InDeltaTime;
	}

	void CBody::SetDirty(const bool Dirty)
	{
		bDirty = Dirty;
	}

	glm::vec2 CBody::GetPosition() const
	{
		const b2Vec2 Pos = b2Body_GetPosition(ID);
		return glm::vec2(Pos.x, Pos.y);
	}

	void CBody::SetPosition(const glm::vec2& Pos) const
	{
		b2Body_SetTransform(ID, Math::Convert(Pos), b2Body_GetRotation(ID));
	}

	void CBody::SetPositionX(const float X) const
	{
		const b2Vec2 Pos = b2Body_GetPosition(ID);
		b2Body_SetTransform(ID, b2Vec2(X, Pos.y), b2Body_GetRotation(ID));
	}

	void CBody::SetPositionY(const float Y) const
	{
		const b2Vec2 Pos = b2Body_GetPosition(ID);
		b2Body_SetTransform(ID, b2Vec2(Pos.x, Y), b2Body_GetRotation(ID));
	}

	float CBody::GetRotation() const
	{
		return b2Rot_GetAngle(b2Body_GetRotation(ID));
	}

	void CBody::SetRotation(const float AngleRad) const
	{
		const b2Transform Transform = b2Body_GetTransform(ID);
		b2Body_SetTransform(ID, Transform.p, b2MakeRot(AngleRad));
	}

	glm::vec2 CBody::GetLinearVelocity() const
	{
		const b2Vec2 Velocity = b2Body_GetLinearVelocity(ID);
		return glm::vec2(Velocity.x, Velocity.y);
	}

	void CBody::SetLinearVelocity(const glm::vec2& InVelocity) const
	{
		b2Body_SetLinearVelocity(ID, b2Vec2(InVelocity.x, InVelocity.y));
	}

	float CBody::GetAngularVelocity() const
	{
		return b2Body_GetAngularVelocity(ID);
	}

	void CBody::ApplyForce(const glm::vec2& InForce, const bool bWakeUp) const
	{
		b2Body_ApplyForceToCenter(ID, { InForce.x, InForce.y }, bWakeUp);
	}

	void CBody::ApplyImpulse(const glm::vec2& InImpulse, const bool bWakeUp) const
	{
		b2Body_ApplyLinearImpulseToCenter(ID, { InImpulse.x, InImpulse.y }, bWakeUp);
	}

	float CBody::GetMass() const
	{
		return b2Body_GetMass(ID);
	}
	
	void CBody::SetMass(const float InMass) const
	{
		b2MassData Data{};
		Data.mass = InMass;
		Data.center = { 0.0f, 0.0f };
		Data.rotationalInertia = 0.0f;
		b2Body_SetMassData(ID, Data);
	}

	void CBody::SetShape(const b2Polygon& Polygon)
	{
		ShapeType = EShape::Polygon;
		b2Shape_SetPolygon(ShapeID, &Polygon);
	}

	void CBody::SetShape(const b2Capsule& Capsule)
	{
		ShapeType = EShape::Capsule;
		b2Shape_SetCapsule(ShapeID, &Capsule);
	}

	void CBody::SetShape(const b2Segment& Line)
	{
		ShapeType = EShape::Line;
		b2Shape_SetSegment(ShapeID, &Line);
	}

	void CBody::SetScale(const float Factor)
	{
		SetScale({ Factor, Factor });
	}

	void CBody::SetScale(const glm::vec2& Factor)
	{
		bDirty = true;
		switch (ShapeType)
		{
			case EShape::Polygon:
				ScalePolygon(Factor);
				break;

			case EShape::Line:
				ScaleLine(Factor);
				break;

			case EShape::Capsule:
				ScaleCapsule(Factor);
				break;

			case EShape::None:
				LK_ASSERT(false);
				break;
		}
	}

	glm::vec2 CBody::GetSize() const
	{
		if (ShapeType == EShape::Polygon)
		{
			auto& Ref = std::get<FPolygon>(Shape);
			return GetBoundingBox(Ref);
		}
		else if (ShapeType == EShape::Line)
		{
			auto& Ref = std::get<FLine>(Shape);
			return GetBoundingBox(Ref);
		}
		else if (ShapeType == EShape::Capsule)
		{
			auto& Ref = std::get<FCapsule>(Shape);
			return GetBoundingBox(Ref);
		}

		return { 0.0f, 0.0f };
	}

	void CBody::SetBodyDef(b2BodyDef& BodyDef, const FBodySpecification& Spec) const
	{
		switch (Spec.Type)
		{
			case EBodyType::Static:
				BodyDef.type = b2_staticBody;
				break;
			case EBodyType::Dynamic:
				BodyDef.type = b2_dynamicBody;
				break;
			case EBodyType::Kinematic:
				BodyDef.type = b2_kinematicBody;
				break;
			default:
				LK_VERIFY(false);
		}

		BodyDef.position = Math::Convert(Spec.Position);
		BodyDef.gravityScale = Spec.GravityScale;
		BodyDef.angularDamping = Spec.AngularDamping;
		BodyDef.linearDamping = Spec.LinearDamping;

		/**
		 * @fixme: Improve the handling of the shape reference here.
		 * Should not be needed to check variant twice to make the initial
		 * body definition rotation be set correctly.
		 */
		if (ShapeType == EShape::Polygon)
		{
			const FPolygon& ShapeRef = std::get<FPolygon>(Spec.Shape);
			BodyDef.rotation = b2MakeRot(ShapeRef.Rotation);
			LK_TRACE_TAG("Body", "Rotation: {} rad", ShapeRef.Rotation);
		}
		else if (ShapeType == EShape::Line)
		{
		}
		else if (ShapeType == EShape::Capsule)
		{
		}

		if (Spec.MotionLock != EMotionLock_None)
		{
			if (Spec.MotionLock & EMotionLock_X)
			{
				BodyDef.motionLocks.linearX = true;
				LK_TRACE_TAG("Body", "Motion lock: X");
			}
			if (Spec.MotionLock & EMotionLock_Y)
			{
				BodyDef.motionLocks.linearY = true;
				LK_TRACE_TAG("Body", "Motion lock: Y");
			}
			if (Spec.MotionLock & EMotionLock_Z)
			{
				BodyDef.motionLocks.angularZ = true;
				LK_TRACE_TAG("Body", "Motion lock: Z");
			}
		}
	}

	void CBody::ScalePolygon(const glm::vec2& Factor) const
	{
		LK_ASSERT(ShapeType == EShape::Polygon);
		b2Polygon Shape = b2Shape_GetPolygon(ShapeID);
		for (int Idx = 0; Idx < Shape.count; Idx++)
		{
			Shape.vertices[Idx].x *= Factor.x;
			Shape.vertices[Idx].y *= Factor.y;
			LK_DEBUG_TAG("Body", "Vertex[{}]: ({}, {})", Idx, Shape.vertices[Idx].x, Shape.vertices[Idx].y);
		}
		Shape.radius *= Factor.x; /* @fixme: Determine way to unify the use of xy here */
		LK_DEBUG_TAG("Body", "Polygon radius: {}", Shape.radius);

		b2Shape_SetPolygon(ShapeID, &Shape);
	}

	void CBody::ScaleLine(const glm::vec2& Factor) const
	{
		LK_ASSERT(ShapeType == EShape::Line);
		b2Segment Shape = b2Shape_GetSegment(ShapeID);
		Shape.point1.x *= Factor.x;
		Shape.point1.y *= Factor.y;
		Shape.point2.x *= Factor.x;
		Shape.point2.y *= Factor.y;
	}

	void CBody::ScaleCapsule(const glm::vec2& Factor) const
	{
		LK_ASSERT(ShapeType == EShape::Capsule);
		b2Capsule Shape = b2Shape_GetCapsule(ShapeID);
		Shape.center1.x *= Factor.x;
		Shape.center1.y *= Factor.y;
		Shape.center2.x *= Factor.x;
		Shape.center2.y *= Factor.y;
		Shape.radius *= Factor.x; /* @fixme: Determine way to unify the use of xy here */
		LK_DEBUG_TAG("Body", "New capsule radius: {}", Shape.radius);

		b2Shape_SetCapsule(ShapeID, &Shape);
	}

	float CBody::GetRestitution() const
	{
		return b2Shape_GetRestitution(ShapeID);
	}

	void CBody::SetRestitution(const float Restitution) const
	{
		b2Shape_SetRestitution(ShapeID, Restitution);
	}

	float CBody::GetFriction() const
	{
		return b2Shape_GetFriction(ShapeID);
	}

	void CBody::SetFriction(const float Friction) const
	{
		b2Shape_SetFriction(ShapeID, Friction);
	}

}