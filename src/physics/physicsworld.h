#pragma once

#include <box2d/box2d.h>
#include <glm/glm.hpp>

#include "core/core.h"
#include "body.h"

namespace platformer2d {

	class CPhysicsWorld
	{
	public:
		CPhysicsWorld() = delete;
		~CPhysicsWorld() = delete;

		static void Initialize(const glm::vec2& Gravity = {0.0f, -10.0f});
		static void Destroy();

		static void Update(float DeltaTime);

		static b2BodyId CreateBody(const b2BodyDef& BodyDef);

		static inline const b2WorldId& GetWorldID() { return WorldID; }

	private:
		static inline b2WorldId WorldID;
		static inline int Substep = 4;
	};

}