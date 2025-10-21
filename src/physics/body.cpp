#include "body.h"

#include "physicsworld.h"

namespace platformer2d {

	CBody::CBody(const FActorSpecification& Spec)
		: Type(Spec.BodyType)
	{
		LK_DEBUG_TAG("Body", "New body: {}", static_cast<int>(Type));
		b2BodyDef BodyDef = b2DefaultBodyDef();
		BodyDef.position = { Spec.Position.x, Spec.Position.y };
		//BodyDef.gravityScale = Spec.GravityScale;
		BodyDef.name = Spec.Name.c_str();
		//BodyDef.linearDamping = 1.0f;

		b2BodyType BodyType = b2_staticBody;
		switch (Type)
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
		}

		ID = CPhysicsWorld::CreateBody(BodyDef);

		/** @fixme */
		//b2Polygon Box = b2MakeBox(1.0f, 1.0f);
		b2Polygon Box = b2MakeBox(Spec.Size.x * 0.50f, Spec.Size.y * 0.50f);

		b2ShapeDef ShapeDef = b2DefaultShapeDef();
		ShapeDef.density = Spec.Density;
		ShapeDef.material.friction = Spec.Friction;
		ShapeDef.material.restitution = Spec.Restitution;
		//ShapeDef.isSensor = false;
		//ShapeDef.enableContactEvents = true;
		//ShapeDef.enableHitEvents = true;

		b2ShapeId ShapeID = b2CreatePolygonShape(ID, &ShapeDef, &Box);
		//b2MassData MassData = b2Body_GetMassData(ID);
	}

	void CBody::Tick(const float InDeltaTime)
	{
		DeltaTime = InDeltaTime;
	}

	void CBody::SetPosition(const glm::vec2& Pos) const
	{
		//b2Body_SetTransform(ID, b2Vec2(Pos.x, Pos.y), b2Body_GetRotation(ID));
		const b2Transform Transform = { b2Vec2(Pos.x, Pos.y), b2MakeRot(B2_PI) };
		b2Body_SetTargetTransform(ID, Transform, DeltaTime);
	}

	void CBody::SetPositionX(const float X) const
	{
		const b2Vec2 Pos = b2Body_GetPosition(ID);
		//b2Body_SetTransform(ID, b2Vec2(X, Pos.y), b2Body_GetRotation(ID));
		const b2Transform Transform = { b2Vec2(X, Pos.y), b2MakeRot(B2_PI) };
		b2Body_SetTargetTransform(ID, Transform, DeltaTime);
	}

	void CBody::SetPositionY(const float Y) const
	{
		const b2Vec2 Pos = b2Body_GetPosition(ID);
		//b2Body_SetTransform(ID, b2Vec2(Pos.x, Y), b2Body_GetRotation(ID));
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
		//b2Body_SetTransform(ID, b2Body_GetPosition(ID), b2MakeRot(AngleRad));
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