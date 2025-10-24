#pragma once

#include <box2d/box2d.h>
#include <glm/glm.hpp>

#include "core/core.h"
#include "scene/actorspecification.h"

namespace platformer2d {

	class CBody
	{
	public:
		CBody(const FActorSpecification& Spec);
		~CBody() = default;

		void Tick(float InDeltaTime);

		const b2BodyId& GetID() const { return ID; }
		glm::vec2 GetPosition() const;
		void SetPosition(const glm::vec2& Pos) const;
		void SetPositionX(float X) const;
		void SetPositionY(float Y) const;

		float GetRotation() const;
		void SetRotation(float AngleRad) const;
		glm::vec2 GetLinearVelocity() const;
		void SetLinearVelocity(const glm::vec2& InVelocity) const;

		void ApplyForce(const glm::vec2& InForce, bool bWakeUp = true) const;
		void ApplyImpulse(const glm::vec2& InImpulse, bool bWakeUp = true) const;

		float GetMass() const;
		void SetMass(float InMass) const;

		void SetShape(const b2Polygon& Polygon);
		void SetShape(const b2Capsule& Capsule);
		void SetShape(const b2Segment& Line);

		void SetScale(float Factor) const;
		void SetRestitution(float Restitution) const;
		void SetFriction(float Friction) const;

	private:
		void ScalePolygon(float Factor) const;
		void ScaleLine(float Factor) const;
		void ScaleCapsule(float Factor) const;

	private:
		b2BodyId ID;
		b2ShapeId ShapeID; /* @todo: Should support multiple shapes */
		EShape ShapeType;

		float DeltaTime = 0.0f;

		friend class CPhysicsWorld;
	};

}
