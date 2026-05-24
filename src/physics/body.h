#pragma once

#include <span>

#include <box2d/box2d.h>
#include <glm/glm.hpp>

#include "bodytype.h"
#include "core/core.h"
#include "core/math/aabb.h"
#include "core/math/shapes.h"
#include "serialization/serializable.h"

namespace platformer2d {

	class CActor;

	enum EMotionLock : std::uint32_t
	{
		EMotionLock_None = 0,
		EMotionLock_X = LK_BIT(1),
		EMotionLock_Y = LK_BIT(2),
		EMotionLock_Z = LK_BIT(3),
		EMotionLock_All = (EMotionLock_X | EMotionLock_Y | EMotionLock_Z)
	};

	enum EBodyFlag : std::uint32_t
	{
		EBodyFlag_None = 0,
		EBodyFlag_PreSolveEvents = LK_BIT(1),
		EBodyFlag_ContactEvents = LK_BIT(2),
		EBodyFlag_SensorEvents = LK_BIT(3),
		EBodyFlag_Bullet = LK_BIT(4),
	};

	struct FBodySpecification
	{
		EBodyType Type = EBodyType::Static;
		TShape Shape{};

		glm::vec2 Position = {0.0f, 0.0f};
		float Friction = 0.60f;
		float Density = 1.0f;
		float GravityScale = 1.0f;
		glm::vec2 LinearVelocity = {0.0f, 0.0f};
		float LinearDamping = 0.0f;
		float AngularVelocity = 0.0f;
		float AngularDamping = 0.0f;
		float DirForce = 4.560f;
		float JumpImpulse = 0.480f;
		std::underlying_type_t<EBodyFlag> Flags = EBodyFlag_None;
		std::underlying_type_t<EMotionLock> MotionLock = EMotionLock_None;
		bool bSensor = false;

		void* UserData = nullptr;
	};

	class CBody : public ISerializable<ESerializable::Yaml>
	{
	public:
		CBody(const FBodySpecification& Spec, CActor* Owner);
		CBody() = delete;
		CBody(CBody&&) = delete;
		CBody(const CBody&) = delete;
		~CBody();

		CBody& operator=(CBody&&) = delete;
		CBody& operator=(const CBody&) = delete;

		void Tick(float InDeltaTime);

		[[nodiscard]] const b2BodyId& GetID() const { return ID; }
		[[nodiscard]] const b2ShapeId& GetShapeID() const { return ShapeID; }
		[[nodiscard]] const b2ChainId& GetChainID() const { return ChainID; }
		[[nodiscard]] EBodyType GetType() const;
		[[nodiscard]] const FBodySpecification& GetSpecification() const { return BodySpec; }

		bool IsEnabled() const;
		void SetEnabled(bool Enabled) const;
		bool IsDirty() const { return bDirty; }
		void SetDirty(bool Dirty);
		bool IsAwake() const;
		void SetAwake(bool Awake) const;
		bool IsSensor() const;

		glm::vec2 GetPosition() const;
		void SetPosition(const glm::vec2& Pos) const;
		void SetPositionX(float X) const;
		void SetPositionY(float Y) const;

		float GetRotation() const;
		void SetRotation(float AngleRad) const;
		glm::vec2 GetLinearVelocity() const;
		void SetLinearVelocity(const glm::vec2& InVelocity) const;
		float GetAngularVelocity() const;
		void SetAngularVelocity(float InVelocity) const;

		void ApplyForce(const glm::vec2& InForce, bool bWakeUp = true) const;
		void ApplyImpulse(const glm::vec2& InImpulse, bool bWakeUp = true) const;

		float GetMass() const;
		void SetMass(float InMass) const;

		void SetShape(const b2Polygon& Polygon);
		void SetShape(const b2Capsule& Capsule);
		void SetShape(const b2Segment& Line);
		bool Rebuild();

		/**
		 * @brief Update the size of the underlying shape in-place.
		 * Currently supported for polygon shapes.
		 */
		void SetSize(const glm::vec2& InSize);

		/**
		 * @brief Replace the body's underlying spec, destroying and recreating
		 * the box2d body and shape with the new parameters.
		 */
		void Replace(const FBodySpecification& NewSpec, CActor* Owner);

		/**
		 * @brief Update the chain's points in place.
		 * Destroys the current chain and rebuilds it from the new points.
		 * Requires at least 4 points.
		 */
		void SetChainPoints(std::span<const glm::vec2> NewPoints, bool bLoop, bool bBlockBothSides);

		void SetScale(float Factor);
		void SetScale(const glm::vec2& Factor);
		float GetRestitution() const;
		void SetRestitution(float Restitution) const;
		float GetFriction() const;
		void SetFriction(float Friction) const;

		[[nodiscard]] const TShape& GetShape() const { return Shape; }

		/**
		 * @brief Safe shape accessor.
		 */
		template<EShape T>
		const TShapeType<T>* TryGetShape() const noexcept
		{
			return std::get_if<TShapeType<T>>(&Shape);
		}

		template<EShape T>
		TShapeType<T>& GetShape()
		{
			using ShapeClass = TShapeType<T>;
			return std::get<ShapeClass>(Shape);
		}

		template<EShape T>
		const TShapeType<T>& GetShape() const
		{
			using ShapeClass = TShapeType<T>;
			return std::get<ShapeClass>(Shape);
		}

		glm::vec2 GetSize() const;
		FAABB GetAABB() const;

		bool Serialize(YAML::Emitter& Out, EExtendableSerializer Extendable = EExtendableSerializer::No) const override;

		[[nodiscard]] static std::string ToString(const FBodySpecification& Spec);

	public:
		static constexpr float LINEAR_VELOCITY_X_EPSILON = 0.010f;
		static constexpr float LINEAR_VELOCITY_Y_EPSILON = 0.050f;

	private:
		void SetBodyDef(b2BodyDef& BodyDef, const FBodySpecification& Spec) const;

		void ScalePolygon(const glm::vec2& Factor);
		void ScaleLine(const glm::vec2& Factor);
		void ScaleCapsule(const glm::vec2& Factor);
		void RebuildPolygon();

		void Build(const FBodySpecification& Spec, CActor* Owner);

		static EBodyType DetermineBodyType(b2BodyType Type);

	private:
		/**
		 * @todo: Need to figure out how the spec should be used.
		 * I don't want to duplicate values but I also do not
		 * want to use it as mutable during the lifetime of the body.
		 */
		FBodySpecification BodySpec;
		b2BodyId ID = b2_nullBodyId;
		b2ShapeId ShapeID = b2_nullShapeId; /* @todo: Should support multiple shapes */
		b2ShapeDef ShapeDef;
		TShape Shape;
		EShape ShapeType = EShape::None;

		/**
		 * @todo: Not a big fan of this at all... I would like to aggregate chains to a body in a better way.
		 * Should be possible to do this once multiple shapes are supported.
		 */
		b2ChainId ChainID = b2_nullChainId;
		b2ChainId ChainID2 = b2_nullChainId; /* Twin for two-sided chains. */

		float DeltaTime = 0.0f;
		bool bDirty = false;

		float GravityScale = 1.0f;

		friend class CPhysicsWorld;
	};

}
