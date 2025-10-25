#pragma once

#include <box2d/box2d.h>
#include <glm/glm.hpp>

#include "core/core.h"
#include "scene/actorspecification.h"

namespace platformer2d {

	enum EMotionLock
	{
		EMotionLock_None = 0,
		EMotionLock_X    = LK_BIT(1),
		EMotionLock_Y    = LK_BIT(2),
		EMotionLock_Z    = LK_BIT(3),
		EMotionLock_All  = (EMotionLock_X | EMotionLock_Y | EMotionLock_Z)
	};

	enum EBodyFlag
	{
		EBodyFlag_None = 0,
		EBodyFlag_PreSolveEvents = LK_BIT(1),
		EBodyFlag_ContactEvents  = LK_BIT(2),
		EBodyFlag_SensorEvents   = LK_BIT(3),
		EBodyFlag_IsBullet       = LK_BIT(4),
	};

	struct FBodySpecification
	{
		EBodyType Type = EBodyType::Static;
		TShape Shape{};

		glm::vec2 Position = { 0.0f, 0.0f };
		float Friction = 0.60f;
		float Density = 1.0f;
		glm::vec2 LinearVelocity = { 0.0f, 0.0f };
		float AngularVelocity = 0.0f;
		float GravityScale = 1.0f;
		float LinearDamping = 0.0f;
		float AngularDamping = 0.0f;
		bool bSensor = false;
		EBodyFlag Flags = EBodyFlag_None;
		EMotionLock MotionLock = EMotionLock_None;

		void* UserData = nullptr;
	};

	class CBody
	{
	public:
		CBody(const FBodySpecification& Spec);
		[[deprecated("Replaced by FBodySpecification")]] CBody(const FActorSpecification& ActorSpec);
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
