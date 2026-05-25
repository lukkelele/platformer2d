#pragma once

#include <box2d/box2d.h>
#include <glm/glm.hpp>

#include "core/core.h"
#include "core/delegate.h"
#include "body.h"
#include "events.h"

namespace platformer2d {

	using TPreSolveFunc = bool (*)(b2ShapeId ShapeA, b2ShapeId ShapeB, b2Vec2 Point, b2Vec2 Normal, void* Ctx);

	class CPhysicsWorld
	{
	public:
		LK_DECLARE_EVENT(FOnSensorBeginEvent, CPhysicsWorld, const CSensorBeginEvent&);
		LK_DECLARE_EVENT(FOnSensorEndEvent, CPhysicsWorld, const CSensorEndEvent&);
		LK_DECLARE_EVENT(FOnContactBeginEvent, CPhysicsWorld, const CContactBeginEvent&);
		LK_DECLARE_EVENT(FOnContactEndEvent, CPhysicsWorld, const CContactEndEvent&);

	public:
		CPhysicsWorld() = delete;
		~CPhysicsWorld() = delete;

		static void Initialize(const glm::vec2& Gravity = {0.0f, -10.0f});
		static void Destroy();

		static void Update(float DeltaTime);
		static void Pause();
		static void Unpause();
		static bool IsPaused();

		static bool IsValid() { return b2World_IsValid(WorldID); }
		static inline const b2WorldId& GetID() { return WorldID; }
		static void SetPreSolve(TPreSolveFunc InPreSolve, void* Context);

		static b2BodyId CreateBody(const b2BodyDef& BodyDef);
		static void Destroy(CBody& Body);

		static glm::vec2 GetGravity();
		static void SetGravity(const glm::vec2& Gravity);
		static std::uint8_t GetSubstep() { return Substep; }

	private:
		static bool PreSolve(b2ShapeId ShapeA, b2ShapeId ShapeB, b2Vec2 Point, b2Vec2 Normal, void* Ctx);
		static void HandleSensorEvents();
		static void HandleContactEvents();

	public:
		static inline FOnSensorBeginEvent OnSensorBeginEvent;
		static inline FOnSensorEndEvent OnSensorEndEvent;
		static inline FOnContactBeginEvent OnContactBeginEvent;
		static inline FOnContactEndEvent OnContactEndEvent;

	private:
		static inline b2WorldId WorldID;
		static inline uint8_t Substep = 6;
	};

}
