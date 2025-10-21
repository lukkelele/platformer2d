#pragma once

#include <box2d/box2d.h>
#include <glm/glm.hpp>

#include "core/core.h"
#include "scene/actorspecification.h"

namespace platformer2d {

#if 0
	struct FBodySpecification
	{
		EBodyType Type = EBodyType::Static;
		glm::vec2 Position = { 0.0f, 0.0f };
		glm::vec2 Size = { 1.0f, 1.0f };
		float Density = 1.0f;
		float Friction = 0.30f;
		float Restitution = 0.0f;
	};
#endif

	class CBody
	{
	public:
		CBody(const FActorSpecification& Spec);
		~CBody() = default;

		void Tick(float InDeltaTime);

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
		EBodyType Type;
		b2BodyId ID;

		float DeltaTime = 0.0f;
	};

}
