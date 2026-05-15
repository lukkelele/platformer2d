#pragma once

#include <glm/glm.hpp>

#include "physics/body.h"
#include "physics/bodytype.h"

namespace platformer2d::UI {

	struct FPhysicsBodyData
	{
		EBodyType BodyType = EBodyType::Static;
		glm::vec3 Position = {0.0f, 0.0f, 0.0f};
		float Friction = 0.60f;
		float Density = 1.0f;
		glm::vec2 LinearVelocity = {0.0f, 0.0f};
		float AngularVelocity = 0.0f;
		float GravityScale = 1.0f;
		float LinearDamping = 0.0f;
		float AngularDamping = 0.0f;
		float DirForce = 5.630f;
		float JumpImpulse = 0.530f;
		bool bSensor = false;

		struct
		{
			bool bPreSolveEvents = true;
			bool bContactEvents = false;
			bool bSensorEvents = false;
			bool bBullet = false;
		} BodyFlag;

		struct
		{
			bool X = false;
			bool Y = false;
			bool Z = false;
			bool All = false;
		} MotionLock;
	};

	extern FPhysicsBodyData PhysicsBodyData;

	void Aggregate(const FPhysicsBodyData& Data, FBodySpecification& BodySpec);
	void PhysicsBodyMenu(FPhysicsBodyData& Data);

}
