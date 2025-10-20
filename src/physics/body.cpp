#include "body.h"

#include "physicsworld.h"

namespace platformer2d {

	CBody::CBody(const EBodyType InType)
		: Type(InType)
	{
		b2BodyDef BodyDef = b2DefaultBodyDef();
		ID = CPhysicsWorld::CreateBody(BodyDef);

		b2BodyType BodyType = b2BodyType::b2_staticBody;
		switch (Type)
		{
			case EBodyType::Static:
				BodyType = b2_staticBody;
				break;
			case EBodyType::Dynamic:
				BodyType = b2_dynamicBody;
				break;
			case EBodyType::Kinematic:
				BodyType = b2_kinematicBody;
				break;
		}

		/** @fixme */
		Box = b2MakeBox(0.25f, 0.25f);

		ShapeDef = b2DefaultShapeDef();
		ShapeDef.density = 1.0f;
		ShapeDef.material.friction = 0.30f;

		ShapeID = b2CreatePolygonShape(ID, &ShapeDef, &Box);
	}

	void CBody::SetPosition(const glm::vec2& Pos) const
	{
		b2Body_SetTransform(ID, b2Vec2(Pos.x, Pos.y), b2Body_GetRotation(ID));
	}

	glm::vec2 CBody::GetPosition() const
	{
		const b2Transform Transform = b2Body_GetTransform(ID);
		return glm::vec2(Transform.p.x, Transform.p.y);
	}

	float CBody::GetRotation() const
	{
		return b2Rot_GetAngle(b2Body_GetRotation(ID));
	}

	void CBody::SetRotation(const float AngleRad) const
	{
		b2Body_SetTransform(ID, b2Body_GetPosition(ID), b2MakeRot(AngleRad));
	}

	glm::vec2 CBody::GetLinearVelocity() const
	{
		const b2Vec2 Velocity = b2Body_GetLinearVelocity(ID);
		return glm::vec2(Velocity.x, Velocity.y);
	}

}