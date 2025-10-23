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
			const FPolygon& ShapeRef = std::get<FPolygon>(Spec.Shape);
			b2Polygon Polygon = b2MakeBox(ShapeRef.Size.x * 0.50f, ShapeRef.Size.y * 0.50f);
			ShapeID = b2CreatePolygonShape(ID, &Spec.ShapeDef, &Polygon);
		}
		else if (std::holds_alternative<FCapsule>(Spec.Shape))
		{
			const FCapsule& ShapeRef = std::get<FCapsule>(Spec.Shape);
			const b2Capsule Capsule = { 
				{ ShapeRef.P0.x, ShapeRef.P0.y }, 
				{ ShapeRef.P1.x, ShapeRef.P1.y }, 
				ShapeRef.Radius 
			};
			ShapeID = b2CreateCapsuleShape(ID, &Spec.ShapeDef, &Capsule);
		}
		else if (Spec.ShapeType & EShape::EShape_Segment)
		{
			LK_ASSERT(false);
		}
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

}