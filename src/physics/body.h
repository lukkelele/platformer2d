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
		void SetLinearVelocity(const glm::vec2& Velocity) const;

		void ApplyForce(const glm::vec2& Force, bool bWakeUp = true) const;
		void ApplyImpulse(const glm::vec2& Impulse, bool bWakeUp = true) const;

	private:
		b2BodyId ID;
		b2ShapeId ShapeID;
		float DeltaTime = 0.0f;

		friend class CPhysicsWorld;
	};

}
