#include "body.h"

#include "physicsworld.h"

namespace platformer2d {

	CBody::CBody(const FActorSpecification& Spec)
	{
		ID = CPhysicsWorld::CreateBody(Spec.BodyDef);
		LK_DEBUG_TAG("Body", "New body: {}", static_cast<int>(Spec.BodyDef.type));
		const b2BodyDef& BodyDef = Spec.BodyDef;
		const b2ShapeDef& ShapeDef = Spec.ShapeDef;

		if (std::holds_alternative<FPolygon>(Spec.Shape))
		{
			ShapeType = EShape::Polygon;
			const FPolygon& ShapeRef = std::get<FPolygon>(Spec.Shape);
			b2Polygon Polygon = b2MakeBox(ShapeRef.Size.x * 0.50f, ShapeRef.Size.y * 0.50f);
			ShapeID = b2CreatePolygonShape(ID, &Spec.ShapeDef, &Polygon);
		}
		else if (std::holds_alternative<FLine>(Spec.Shape))
		{
			ShapeType = EShape::Line;
			LK_ASSERT(false);
		}
		else if (std::holds_alternative<FCapsule>(Spec.Shape))
		{
			ShapeType = EShape::Capsule;
			const FCapsule& ShapeRef = std::get<FCapsule>(Spec.Shape);
			const b2Capsule Capsule = { 
				{ ShapeRef.P0.x, ShapeRef.P0.y }, 
				{ ShapeRef.P1.x, ShapeRef.P1.y }, 
				ShapeRef.Radius 
			};
			ShapeID = b2CreateCapsuleShape(ID, &Spec.ShapeDef, &Capsule);
		}

		SetMass(1.0f);
	}

	void CBody::Tick(const float InDeltaTime)
	{
		DeltaTime = InDeltaTime;
	}

	void CBody::SetPosition(const glm::vec2& Pos) const
	{
		const b2Transform Transform = { b2Vec2(Pos.x, Pos.y), b2MakeRot(B2_PI) };
		b2Body_SetTargetTransform(ID, Transform, DeltaTime);
	}

	void CBody::SetPositionX(const float X) const
	{
		const b2Vec2 Pos = b2Body_GetPosition(ID);
		const b2Transform Transform = { b2Vec2(X, Pos.y), b2MakeRot(B2_PI) };
		b2Body_SetTargetTransform(ID, Transform, DeltaTime);
	}

	void CBody::SetPositionY(const float Y) const
	{
		const b2Vec2 Pos = b2Body_GetPosition(ID);
		const b2Transform Transform = { b2Vec2(Pos.x, Y), b2MakeRot(B2_PI) };
		b2Body_SetTargetTransform(ID, Transform, DeltaTime);
	}

	glm::vec2 CBody::GetPosition() const
	{
		const b2Vec2 Pos = b2Body_GetPosition(ID);
		return glm::vec2(Pos.x, Pos.y);
	}

	float CBody::GetRotation() const
	{
		return b2Rot_GetAngle(b2Body_GetRotation(ID));
	}

	void CBody::SetRotation(const float AngleRad) const
	{
		const b2Vec2 Pos = b2Body_GetPosition(ID);
		const b2Transform Transform = { Pos, b2MakeRot(AngleRad) };
		b2Body_SetTargetTransform(ID, Transform, DeltaTime);
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

	void CBody::SetScale(const float Factor) const
	{
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

	void CBody::ScalePolygon(const float Factor) const
	{
		LK_ASSERT(ShapeType == EShape::Polygon);
		b2Polygon Shape = b2Shape_GetPolygon(ShapeID);
		for (int Idx = 0; Idx < Shape.count; Idx++)
		{
			Shape.vertices[Idx].x *= Factor;
			Shape.vertices[Idx].y *= Factor;
		}
		Shape.radius *= Factor;
		LK_DEBUG_TAG("Body", "New polygon radius: {}", Shape.radius);

		b2Shape_SetPolygon(ShapeID, &Shape);
	}

	void CBody::ScaleLine(const float Factor) const
	{
		LK_ASSERT(ShapeType == EShape::Line);
		b2Segment Shape = b2Shape_GetSegment(ShapeID);
		Shape.point1.x *= Factor;
		Shape.point1.y *= Factor;
		Shape.point2.x *= Factor;
		Shape.point2.y *= Factor;
	}

	void CBody::ScaleCapsule(const float Factor) const
	{
		LK_ASSERT(ShapeType == EShape::Capsule);
		b2Capsule Shape = b2Shape_GetCapsule(ShapeID);
		Shape.center1.x *= Factor;
		Shape.center1.y *= Factor;
		Shape.center2.x *= Factor;
		Shape.center2.y *= Factor;
		Shape.radius *= Factor;
		LK_DEBUG_TAG("Body", "New capsule radius: {}", Shape.radius);

		b2Shape_SetCapsule(ShapeID, &Shape);
	}

	void CBody::SetRestitution(const float Restitution) const
	{
		b2Shape_SetRestitution(ShapeID, Restitution);
	}

	void CBody::SetFriction(const float Friction) const
	{
		b2Shape_SetFriction(ShapeID, Friction);
	}

}