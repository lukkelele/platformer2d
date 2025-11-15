#pragma once

#include <box2d/box2d.h>
#include <glm/glm.hpp>

#include "core/core.h"
#include "core/math/aabb.h"
#include "core/math/shapes.h"
#include "serialization/serializable.h"

namespace platformer2d {

	enum EMotionLock : uint32_t
	{
		EMotionLock_None = 0,
		EMotionLock_X    = LK_BIT(1),
		EMotionLock_Y    = LK_BIT(2),
		EMotionLock_Z    = LK_BIT(3),
		EMotionLock_All  = (EMotionLock_X | EMotionLock_Y | EMotionLock_Z)
	};

	enum EBodyFlag : uint32_t
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
		float DirForce = 5.630f;
		float JumpImpulse = 0.530f;
		bool bSensor = false;
		EBodyFlag Flags = EBodyFlag_None;
		EMotionLock MotionLock = EMotionLock_None;

		std::string Name{};
		void* UserData = nullptr;
	};

	class CBody : public ISerializable<ESerializable::Yaml>
	{
	public:
		CBody(const FBodySpecification& Spec);
		CBody() = delete;
		~CBody();

		void Tick(float InDeltaTime);

		inline const b2BodyId& GetID() const { return ID; }
		inline const b2ShapeId& GetShapeID() const { return ShapeID; }

		inline bool IsDirty() const { return bDirty; }
		void SetDirty(bool Dirty);

		glm::vec2 GetPosition() const;
		void SetPosition(const glm::vec2& Pos) const;
		void SetPositionX(float X) const;
		void SetPositionY(float Y) const;

		float GetRotation() const;
		void SetRotation(float AngleRad) const;
		glm::vec2 GetLinearVelocity() const;
		void SetLinearVelocity(const glm::vec2& InVelocity) const;
		float GetAngularVelocity() const;

		void ApplyForce(const glm::vec2& InForce, bool bWakeUp = true) const;
		void ApplyImpulse(const glm::vec2& InImpulse, bool bWakeUp = true) const;

		float GetMass() const;
		void SetMass(float InMass) const;

		void SetShape(const b2Polygon& Polygon);
		void SetShape(const b2Capsule& Capsule);
		void SetShape(const b2Segment& Line);

		void SetScale(float Factor);
		void SetScale(const glm::vec2& Factor);
		float GetRestitution() const;
		void SetRestitution(float Restitution) const;
		float GetFriction() const;
		void SetFriction(float Friction) const;

		inline const TShape& GetShape() const { return Shape; }

		/**
		 * @brief Safe shape accessor.
		 */
		template<EShape T>
		const TShapeType<T>* TryGetShape() const noexcept
		{
			return std::get_if<TShapeType<T>>(&Shape);
		}

		template<EShape T>
		const TShapeType<T>& GetShape() const
		{
			using ShapeClass = TShapeType<T>;
			return std::get<ShapeClass>(Shape);
		}

		glm::vec2 GetSize() const;
		FAABB GetAABB() const;

		virtual bool Serialize(YAML::Emitter& Out) const override;

		static std::string ToString(const FBodySpecification& Spec);

	public:
		static constexpr float LINEAR_VELOCITY_X_EPSILON = 0.010f;
		static constexpr float LINEAR_VELOCITY_Y_EPSILON = 0.050f;
	private:
		void SetBodyDef(b2BodyDef& BodyDef, const FBodySpecification& Spec) const;

		void ScalePolygon(const glm::vec2& Factor) const;
		void ScaleLine(const glm::vec2& Factor) const;
		void ScaleCapsule(const glm::vec2& Factor) const;

	private:
		const FBodySpecification BodySpec;
		b2BodyId ID;
		b2ShapeId ShapeID; /* @todo: Should support multiple shapes */
		b2ShapeDef ShapeDef;
		TShape Shape;
		EShape ShapeType;

		bool bDirty = false;
		float DeltaTime = 0.0f;

		friend class CPhysicsWorld;
	};

}
