#include "body.h"

#include "physicsworld.h"

namespace platformer2d {

	CBody::CBody(const FActorSpecification& Spec)
	{
		ID = CPhysicsWorld::CreateBody(Spec.BodyDef);
		LK_DEBUG_TAG("Body", "New body: {}", static_cast<int>(Spec.BodyDef.type));
		const b2BodyDef& BodyDef = Spec.BodyDef;
		const b2ShapeDef& ShapeDef = Spec.ShapeDef;

		if (Spec.ShapeType & EShape::EShape_Polygon)
		{
			b2Polygon Box = b2MakeBox(Spec.Size.x * 0.50f, Spec.Size.y * 0.50f);
			//b2Polygon Box = b2MakeBox(Spec.Size.x, Spec.Size.y);
			ShapeID = b2CreatePolygonShape(ID, &Spec.ShapeDef, &Box);
		}
		else if (Spec.ShapeType & EShape::EShape_Capsule)
		{
			/** @fixme */
			//b2Capsule Capsule = { { 0.0f, 0.0f }, { 0.0f, 1.0f }, Spec.Shape.Capsule.Radius };
			b2Capsule Capsule = { { 0.0f, 0.0f }, { 0.0f, 0.30f }, Spec.Shape.Capsule.Radius };
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

	void CBody::SetLinearVelocity(const glm::vec2& Velocity) const
	{
		b2Body_SetLinearVelocity(ID, b2Vec2(Velocity.x, Velocity.y));
	}

	void CBody::ApplyForce(const glm::vec2& Force, const bool bWakeUp) const
	{
		b2Body_ApplyForceToCenter(ID, { Force.x, Force.y }, bWakeUp);
	}

	void CBody::ApplyImpulse(const glm::vec2& Impulse, const bool bWakeUp) const
	{
		b2Body_ApplyLinearImpulseToCenter(ID, { Impulse.x, Impulse.y }, bWakeUp);
	}

}