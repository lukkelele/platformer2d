#include "body.h"

#include "core/math/math.h"
#include "physicsworld.h"
#include "serialization/yaml.h"

namespace platformer2d {

	CBody::CBody(const FBodySpecification& Spec, CActor* Owner)
	{
		Build(Spec, Owner);

		/* @fixme: Ugly fix since no scaling system is used between box2d scaling and pixels. */
		SetMass(1.0f); /* @todo: Use body spec */
	}

	CBody::~CBody()
	{
		if (b2Body_IsValid(ID)) {
			LK_TRACE_TAG("Body", "Destroy: {}", ID.index1);
			/* Destroying the body destroys all attached shapes and chains. */
			b2DestroyBody(ID);
		}
	}

	void CBody::Tick(const float InDeltaTime)
	{
		DeltaTime = InDeltaTime;
	}

	EBodyType CBody::GetType() const
	{
		return DetermineBodyType(b2Body_GetType(ID));
	}

	bool CBody::IsEnabled() const
	{
		return b2Body_IsEnabled(ID);
	}

	void CBody::SetEnabled(const bool Enabled) const
	{
		if (Enabled) {
			b2Body_Enable(ID);
		} else {
			b2Body_Disable(ID);
		}
	}

	void CBody::SetDirty(const bool Dirty)
	{
		bDirty = Dirty;
	}

	bool CBody::IsAwake() const
	{
		return b2Body_IsAwake(ID);
	}

	void CBody::SetAwake(const bool Awake) const
	{
		b2Body_SetAwake(ID, Awake);
	}

	bool CBody::IsSensor() const
	{
		return BodySpec.bSensor;
	}

	glm::vec2 CBody::GetPosition() const
	{
		const b2Vec2 Pos = b2Body_GetPosition(ID);
		return glm::vec2(Pos.x, Pos.y);
	}

	void CBody::SetPosition(const glm::vec2& Pos) const
	{
		b2Body_SetTransform(ID, Math::Convert(Pos), b2Body_GetRotation(ID));
	}

	void CBody::SetPositionX(const float X) const
	{
		const b2Vec2 Pos = b2Body_GetPosition(ID);
		b2Body_SetTransform(ID, b2Vec2(X, Pos.y), b2Body_GetRotation(ID));
	}

	void CBody::SetPositionY(const float Y) const
	{
		const b2Vec2 Pos = b2Body_GetPosition(ID);
		b2Body_SetTransform(ID, b2Vec2(Pos.x, Y), b2Body_GetRotation(ID));
	}

	float CBody::GetRotation() const
	{
		return b2Rot_GetAngle(b2Body_GetRotation(ID));
	}

	void CBody::SetRotation(const float AngleRad) const
	{
		const b2Transform Transform = b2Body_GetTransform(ID);
		b2Body_SetTransform(ID, Transform.p, b2MakeRot(AngleRad));
	}

	glm::vec2 CBody::GetLinearVelocity() const
	{
		const b2Vec2 Velocity = b2Body_GetLinearVelocity(ID);
		return glm::vec2(Velocity.x, Velocity.y);
	}

	void CBody::SetLinearVelocity(const glm::vec2& InVelocity) const
	{
		b2Body_SetLinearVelocity(ID, b2Vec2(InVelocity.x, InVelocity.y));
	}

	float CBody::GetAngularVelocity() const
	{
		return b2Body_GetAngularVelocity(ID);
	}

	void CBody::SetAngularVelocity(const float InVelocity) const
	{
		b2Body_SetAngularVelocity(ID, InVelocity);
	}

	void CBody::ApplyForce(const glm::vec2& InForce, const bool bWakeUp) const
	{
		b2Body_ApplyForceToCenter(ID, {InForce.x, InForce.y}, bWakeUp);
	}

	void CBody::ApplyImpulse(const glm::vec2& InImpulse, const bool bWakeUp) const
	{
		b2Body_ApplyLinearImpulseToCenter(ID, {InImpulse.x, InImpulse.y}, bWakeUp);
	}

	float CBody::GetMass() const
	{
		return b2Body_GetMass(ID);
	}

	void CBody::SetMass(const float InMass) const
	{
		b2MassData Data{};
		Data.mass = InMass;
		Data.center = {0.0f, 0.0f};
		Data.rotationalInertia = 0.0f;
		b2Body_SetMassData(ID, Data);
	}

	void CBody::SetShape(const b2Polygon& Polygon)
	{
		ShapeType = EShape::Polygon;
		b2Shape_SetPolygon(ShapeID, &Polygon);
	}

	void CBody::SetShape(const b2Capsule& Capsule)
	{
		ShapeType = EShape::Capsule;
		b2Shape_SetCapsule(ShapeID, &Capsule);
	}

	void CBody::SetShape(const b2Segment& Line)
	{
		ShapeType = EShape::Line;
		b2Shape_SetSegment(ShapeID, &Line);
	}

	void CBody::SetSize(const glm::vec2& InSize)
	{
		LK_ASSERT((InSize.x > 0.0f) && (InSize.y > 0.0f), "Invalid size");
		switch (ShapeType) {
			case EShape::Polygon:
			{
				FPolygon& ShapeRef = std::get<FPolygon>(Shape);
				ShapeRef.Size = InSize;
				BodySpec.Shape.emplace<FPolygon>(ShapeRef);

				const b2Polygon Polygon = b2MakeBox(InSize.x * 0.50f, InSize.y * 0.50f);
				b2Shape_SetPolygon(ShapeID, &Polygon);

				if (b2Body_GetType(ID) == b2_dynamicBody) {
					b2Body_ApplyMassFromShapes(ID);
				}

				bDirty = true;
				break;
			}
			case EShape::Capsule:
			{
				/* Uniformly scale the capsule along its existing axis. */
				FCapsule& ShapeRef = std::get<FCapsule>(Shape);
				const glm::vec2 Bounds = GetBoundingBox(ShapeRef);
				const float ScaleX = (Bounds.x > 0.0f) ? (InSize.x / Bounds.x) : 1.0f;
				const float ScaleY = (Bounds.y > 0.0f) ? (InSize.y / Bounds.y) : 1.0f;
				ScaleCapsule({ScaleX, ScaleY});
				break;
			}
			case EShape::Chain:
			case EShape::Line:
			case EShape::None:
				LK_MARK_NOT_IMPLEMENTED();
				break;
		}
	}

	void CBody::Replace(const FBodySpecification& NewSpec, CActor* Owner)
	{
		LK_TRACE_TAG("Body", "Replace: {}", b2Body_IsValid(ID) ? ID.index1 : -1);
		if (b2Body_IsValid(ID)) {
			b2DestroyBody(ID);
			ID = b2_nullBodyId;
			ShapeID = b2_nullShapeId;
			ChainID = b2_nullChainId;
			ChainID2 = b2_nullChainId;
		}

		Build(NewSpec, Owner);

		/* @fixme: Mirror constructor behavior until mass is sourced from spec. */
		if (ShapeType != EShape::Chain) {
			SetMass(1.0f);
		}
	}

	void CBody::SetChainPoints(std::span<const glm::vec2> NewPoints, const bool bLoop, const bool bBlockBothSides)
	{
		LK_ASSERT(ShapeType == EShape::Chain, "Body is not a chain");
		LK_ASSERT(NewPoints.size() >= 4, "Chain requires at least 4 points");
		FChain& LocalRef = std::get<FChain>(Shape);
		LocalRef.Points.assign(NewPoints.begin(), NewPoints.end());
		LocalRef.bLoop = bLoop;
		LocalRef.bBlockBothSides = bBlockBothSides;
		BodySpec.Shape.emplace<FChain>(LocalRef);

		if (b2Chain_IsValid(ChainID)) {
			b2DestroyChain(ChainID);
			ChainID = b2_nullChainId;
		}
		if (b2Chain_IsValid(ChainID2)) {
			b2DestroyChain(ChainID2);
			ChainID2 = b2_nullChainId;
		}

		const b2SurfaceMaterial Material = {
			.friction = LocalRef.Friction,
		};

		/* Primary chain. */
		constexpr bool REVERSED = true;
		std::vector<b2Vec2> Primary = Utility::MakeBox2DChainPoints(NewPoints, bLoop, REVERSED);
		b2ChainDef ChainDef = b2DefaultChainDef();
		ChainDef.points = Primary.data();
		ChainDef.count = static_cast<int>(Primary.size());
		ChainDef.materials = &Material;
		ChainDef.materialCount = 1;
		ChainDef.isLoop = bLoop;
		ChainDef.userData = ShapeDef.userData;
		ChainID = b2CreateChain(ID, &ChainDef);

		/**
		 * Twin chain for two-sided collision.
		 *
		 * @fixme: This could probably be solved some other way that isn't this clunky
		 * I really need to fix this entire class. It is growing large and ugly.
		 */
		constexpr bool NOT_REVERSED = true;
		if (bBlockBothSides) {
			std::vector<b2Vec2> Secondary = Utility::MakeBox2DChainPoints(NewPoints, bLoop, NOT_REVERSED);
			b2ChainDef SecondaryDef = b2DefaultChainDef();
			SecondaryDef.points = Secondary.data();
			SecondaryDef.count = static_cast<int>(Secondary.size());
			SecondaryDef.materials = &Material;
			SecondaryDef.materialCount = 1;
			SecondaryDef.isLoop = bLoop;
			SecondaryDef.userData = ShapeDef.userData;
			ChainID2 = b2CreateChain(ID, &SecondaryDef);
		}

		bDirty = true;
	}

	bool CBody::Rebuild()
	{
		bool Ret = false;
		switch (ShapeType) {
			case EShape::Polygon:
				RebuildPolygon();
				Ret = true;
				break;
			case EShape::Line:
				break;
			case EShape::Capsule:
				break;
			case EShape::Chain:
			{
				const FChain& ChainRef = std::get<FChain>(Shape);
				SetChainPoints(ChainRef.Points, ChainRef.bLoop, ChainRef.bBlockBothSides);
				Ret = true;
				break;
			}
			case EShape::None:
				LK_ASSERT(false);
				break;
		}

		return Ret;
	}

	void CBody::SetScale(const float Factor)
	{
		SetScale({Factor, Factor});
	}

	void CBody::SetScale(const glm::vec2& Factor)
	{
		bDirty = true;
		switch (ShapeType) {
			case EShape::Polygon:
				ScalePolygon(Factor);
				break;
			case EShape::Line:
				ScaleLine(Factor);
				break;
			case EShape::Capsule:
				ScaleCapsule(Factor);
				break;
			case EShape::Chain:
			{
				FChain& LocalRef = std::get<FChain>(Shape);
				for (glm::vec2& P : LocalRef.Points) {
					P.x *= Factor.x;
					P.y *= Factor.y;
				}
				SetChainPoints(LocalRef.Points, LocalRef.bLoop, LocalRef.bBlockBothSides);
				break;
			}
			case EShape::None:
				LK_ASSERT(false);
				break;
		}

		if (b2Body_GetType(ID) == b2_dynamicBody) {
			b2Body_ApplyMassFromShapes(ID);
		}
	}

	float CBody::GetRestitution() const
	{
		return b2Shape_GetRestitution(ShapeID);
	}

	void CBody::SetRestitution(const float Restitution) const
	{
		b2Shape_SetRestitution(ShapeID, Restitution);
	}

	float CBody::GetFriction() const
	{
		return b2Shape_GetFriction(ShapeID);
	}

	void CBody::SetFriction(const float Friction) const
	{
		b2Shape_SetFriction(ShapeID, Friction);
	}

	glm::vec2 CBody::GetSize() const
	{
		if (ShapeType == EShape::Polygon) {
			auto& Ref = std::get<FPolygon>(Shape);
			return Ref.Size;
		} else if (ShapeType == EShape::Line) {
			LK_MARK_NOT_IMPLEMENTED();
			auto& Ref = std::get<FLine>(Shape);
			return GetBoundingBox(Ref);
		} else if (ShapeType == EShape::Capsule) {
			auto& Ref = std::get<FCapsule>(Shape);
			return GetBoundingBox(Ref);
		} else if (ShapeType == EShape::Chain) {
			auto& Ref = std::get<FChain>(Shape);
			return GetBoundingBox(Ref);
		}

		return {0.0f, 0.0f};
	}

	FAABB CBody::GetAABB() const
	{
		if (ShapeType == EShape::Chain) {
			const FChain& ChainRef = std::get<FChain>(Shape);
			if (ChainRef.Points.empty()) {
				return FAABB{glm::vec2(0.0f), glm::vec2(0.0f)};
			}
			glm::vec2 Min = ChainRef.Points.front();
			glm::vec2 Max = ChainRef.Points.front();
			for (const glm::vec2& P : ChainRef.Points) {
				Min = glm::min(Min, P);
				Max = glm::max(Max, P);
			}
			const glm::vec2 BodyPos = GetPosition();
			return FAABB{Min + BodyPos, Max + BodyPos};
		}

		const b2AABB AABB = b2Shape_GetAABB(ShapeID);
		const glm::vec2 Min = glm::vec2(AABB.lowerBound.x, AABB.lowerBound.y);
		const glm::vec2 Max = glm::vec2(AABB.upperBound.x, AABB.upperBound.y);
		return FAABB{Min, Max};
	}

	bool CBody::Serialize(YAML::Emitter& Out, const EExtendableSerializer Extendable) const
	{
		Out << YAML::Key << "Type" << YAML::Value << static_cast<int>(std::to_underlying(BodySpec.Type));
		Out << YAML::Key << "GravityScale" << YAML::Value << GravityScale;

		/* Shape */
		Out << YAML::Key << "Shape";
		Out << YAML::BeginMap;
		Out << YAML::Key << "ShapeType" << YAML::Value << std::to_underlying(ShapeType);
		switch (ShapeType) {
			case EShape::Polygon:
			{
				const auto& ShapeRef = std::get<FPolygon>(Shape);
				Out << YAML::Key << "Size" << YAML::Value << ShapeRef.Size;
				Out << YAML::Key << "Rotation" << YAML::Value << GetRotation();
				Out << YAML::Key << "Radius" << YAML::Value << ShapeRef.Radius;
				LK_TRACE_TAG("Body", "Polygon: Size={} Rotation={} Radius={}", ShapeRef.Size, ShapeRef.Rotation, ShapeRef.Radius);
				break;
			}

			case EShape::Line:
			{
				LK_MARK_NOT_IMPLEMENTED();
				break;
			}

			case EShape::Capsule:
			{
				const auto& ShapeRef = std::get<FCapsule>(Shape);
				Out << YAML::Key << "P0" << YAML::Value << ShapeRef.P0;
				Out << YAML::Key << "P1" << YAML::Value << ShapeRef.P1;
				Out << YAML::Key << "Radius" << YAML::Value << ShapeRef.Radius;
				break;
			}

			case EShape::Chain:
			{
				const auto& ShapeRef = std::get<FChain>(Shape);
				Out << YAML::Key << "Loop" << YAML::Value << ShapeRef.bLoop;
				Out << YAML::Key << "BlockBothSides" << YAML::Value << ShapeRef.bBlockBothSides;
				Out << YAML::Key << "Friction" << YAML::Value << ShapeRef.Friction;
				Out << YAML::Key << "Points" << YAML::Value << YAML::BeginSeq;
				for (const glm::vec2& P : ShapeRef.Points) {
					Out << P;
				}
				Out << YAML::EndSeq;
				break;
			}

			case EShape::None:
				break;
		}
		Out << YAML::EndMap;
		/* ~Shape */

		Out << YAML::Key << "Position" << YAML::Value << GetPosition();
		Out << YAML::Key << "Flags" << YAML::Value << static_cast<uint32_t>(BodySpec.Flags);
		Out << YAML::Key << "Mass" << YAML::Value << GetMass();
		Out << YAML::Key << "MotionLock" << YAML::Value << static_cast<uint32_t>(BodySpec.MotionLock);
		Out << YAML::Key << "Sensor" << YAML::Value << IsSensor();

		return true;
	}

	std::string CBody::ToString(const FBodySpecification& Spec)
	{
		const EShape ShapeType = DetermineShapeType(Spec.Shape);
		return Format("[BodySpecification] ShapeType={} Pos={} Flags={} MotionLock={} Density={}",
			Enum::ToString(ShapeType), Spec.Position, Spec.Flags, Spec.MotionLock, Spec.Density);
	}

	void CBody::SetBodyDef(b2BodyDef& BodyDef, const FBodySpecification& Spec) const
	{
		switch (Spec.Type) {
			case EBodyType::Static:
				BodyDef.type = b2_staticBody;
				break;
			case EBodyType::Dynamic:
				BodyDef.type = b2_dynamicBody;
				break;
			case EBodyType::Kinematic:
				BodyDef.type = b2_kinematicBody;
				break;
			default:
				LK_VERIFY(false);
		}

		BodyDef.position = Math::Convert(Spec.Position);
		BodyDef.gravityScale = Spec.GravityScale;
		BodyDef.angularDamping = Spec.AngularDamping;
		BodyDef.linearDamping = Spec.LinearDamping;

		/**
		 * @fixme: Improve the handling of the shape reference here.
		 * Should not be needed to check variant twice to make the initial
		 * body definition rotation be set correctly.
		 */
		if (ShapeType == EShape::Polygon) {
			const auto& ShapeRef = std::get<FPolygon>(Spec.Shape);
			BodyDef.rotation = b2MakeRot(ShapeRef.Rotation);
		} else if (ShapeType == EShape::Line) {
			LK_MARK_NOT_IMPLEMENTED();
		} else if (ShapeType == EShape::Capsule) {
			const auto& ShapeRef = std::get<FCapsule>(Spec.Shape);
			LK_UNUSED(ShapeRef);
			BodyDef.rotation = b2MakeRot(0);
		} else if (ShapeType == EShape::Chain) {
			BodyDef.rotation = b2MakeRot(0);
		}

		if (Spec.MotionLock != EMotionLock_None) {
			if (Spec.MotionLock & EMotionLock_X) {
				BodyDef.motionLocks.linearX = true;
				LK_TRACE_TAG("Body", "Motion lock: X");
			}
			if (Spec.MotionLock & EMotionLock_Y) {
				BodyDef.motionLocks.linearY = true;
				LK_TRACE_TAG("Body", "Motion lock: Y");
			}
			if (Spec.MotionLock & EMotionLock_Z) {
				BodyDef.motionLocks.angularZ = true;
				LK_TRACE_TAG("Body", "Motion lock: Z");
			}
		}
	}

	void CBody::ScalePolygon(const glm::vec2& Factor)
	{
		LK_ASSERT(ShapeType == EShape::Polygon);
		FPolygon& LocalRef = std::get<FPolygon>(Shape);
		LocalRef.Size.x *= Factor.x;
		LocalRef.Size.y *= Factor.y;
		LocalRef.Radius *= Factor.x; /* @fixme: Determine way to unify the use of xy here */
		BodySpec.Shape.emplace<FPolygon>(LocalRef);

		b2Polygon Box2DShape = b2Shape_GetPolygon(ShapeID);
		for (int Idx = 0; Idx < Box2DShape.count; Idx++) {
			Box2DShape.vertices[Idx].x *= Factor.x;
			Box2DShape.vertices[Idx].y *= Factor.y;
			LK_DEBUG_TAG("Body", "Vertex[{}]: ({}, {})", Idx, Box2DShape.vertices[Idx].x, Box2DShape.vertices[Idx].y);
		}
		Box2DShape.radius *= Factor.x;
		LK_DEBUG_TAG("Body", "Polygon radius: {}", Box2DShape.radius);

		b2Shape_SetPolygon(ShapeID, &Box2DShape);
	}

	void CBody::ScaleLine(const glm::vec2& Factor)
	{
		LK_ASSERT(ShapeType == EShape::Line);
		FLine& LocalRef = std::get<FLine>(Shape);
		LocalRef.P0.x *= Factor.x;
		LocalRef.P0.y *= Factor.y;
		LocalRef.P1.x *= Factor.x;
		LocalRef.P1.y *= Factor.y;
		BodySpec.Shape.emplace<FLine>(LocalRef);

		b2Segment Box2DShape = b2Shape_GetSegment(ShapeID);
		Box2DShape.point1.x *= Factor.x;
		Box2DShape.point1.y *= Factor.y;
		Box2DShape.point2.x *= Factor.x;
		Box2DShape.point2.y *= Factor.y;
		b2Shape_SetSegment(ShapeID, &Box2DShape);
	}

	void CBody::ScaleCapsule(const glm::vec2& Factor)
	{
		LK_ASSERT(ShapeType == EShape::Capsule);
		FCapsule& LocalRef = std::get<FCapsule>(Shape);
		LocalRef.P0.x *= Factor.x;
		LocalRef.P0.y *= Factor.y;
		LocalRef.P1.x *= Factor.x;
		LocalRef.P1.y *= Factor.y;
		LocalRef.Radius *= Factor.x; /* @fixme: Determine way to unify the use of xy here */
		BodySpec.Shape.emplace<FCapsule>(LocalRef);

		b2Capsule Box2DShape = b2Shape_GetCapsule(ShapeID);
		Box2DShape.center1.x *= Factor.x;
		Box2DShape.center1.y *= Factor.y;
		Box2DShape.center2.x *= Factor.x;
		Box2DShape.center2.y *= Factor.y;
		Box2DShape.radius *= Factor.x;
		LK_DEBUG_TAG("Body", "New capsule radius: {}", Box2DShape.radius);

		b2Shape_SetCapsule(ShapeID, &Box2DShape);
	}

	void CBody::RebuildPolygon()
	{
		LK_ASSERT(ShapeType == EShape::Polygon);
		FPolygon& ShapeRef = std::get<FPolygon>(Shape);

		const float HalfX = ShapeRef.Size.x * 0.50f;
		const float HalfY = ShapeRef.Size.y * 0.50f;

		b2Polygon Polygon = b2MakeBox(HalfX, HalfY);
		Polygon.radius = ShapeRef.Radius;

		/* Destroy and recreate the shape so the body's broad-phase entry rebuilds cleanly. */
		if (b2Shape_IsValid(ShapeID)) {
			b2DestroyShape(ShapeID, false);
		}
		ShapeID = b2CreatePolygonShape(ID, &ShapeDef, &Polygon);
		BodySpec.Shape.emplace<FPolygon>(ShapeRef);

		if (b2Body_GetType(ID) == b2_dynamicBody) {
			b2Body_ApplyMassFromShapes(ID);
		}

		bDirty = true;
	}

	void CBody::Build(const FBodySpecification& Spec, CActor* Owner)
	{
		BodySpec = Spec;
		GravityScale = Spec.GravityScale;
		ShapeType = DetermineShapeType(Spec.Shape);

		b2BodyDef BodyDef = b2DefaultBodyDef();
		SetBodyDef(BodyDef, Spec);

		ShapeDef = b2DefaultShapeDef();
		ShapeDef.userData = Owner;

		if (Spec.Flags & EBodyFlag_PreSolveEvents) {
			ShapeDef.enablePreSolveEvents = true;
		}
		if (Spec.Flags & EBodyFlag_ContactEvents) {
			ShapeDef.enableContactEvents = true;
		}
		if (Spec.Flags & EBodyFlag_SensorEvents) {
			ShapeDef.enableSensorEvents = true;
		}

		ShapeDef.material.friction = Spec.Friction;
		ShapeDef.isSensor = Spec.bSensor;

		ID = CPhysicsWorld::CreateBody(BodyDef);
		Shape = Spec.Shape;

		switch (ShapeType) {
			case EShape::Polygon:
			{
				const FPolygon& ShapeRef = std::get<FPolygon>(Spec.Shape);
				LK_ASSERT((ShapeRef.Size.x > 0.0f) && (ShapeRef.Size.y > 0.0f), "Invalid size");
				const b2Polygon Polygon = b2MakeBox(ShapeRef.Size.x * 0.50f, ShapeRef.Size.y * 0.50f);
				ShapeID = b2CreatePolygonShape(ID, &ShapeDef, &Polygon);
				break;
			}
			case EShape::Capsule:
			{
				const FCapsule& ShapeRef = std::get<FCapsule>(Spec.Shape);
				const b2Capsule Capsule = {
					{ShapeRef.P0.x, ShapeRef.P0.y},
					{ShapeRef.P1.x, ShapeRef.P1.y},
					ShapeRef.Radius
                };
				ShapeID = b2CreateCapsuleShape(ID, &ShapeDef, &Capsule);
				break;
			}
			case EShape::Chain:
			{
				const FChain& ShapeRef = std::get<FChain>(Spec.Shape);
				LK_ASSERT(ShapeRef.Points.size() >= 4, "Chain requires at least 4 points");

				const b2SurfaceMaterial Material = {
					.friction = ShapeRef.Friction,
				};

				constexpr bool REVERSED = true;
				std::vector<b2Vec2> Primary = Utility::MakeBox2DChainPoints(ShapeRef.Points, ShapeRef.bLoop, REVERSED);
				b2ChainDef ChainDef = b2DefaultChainDef();
				ChainDef.points = Primary.data();
				ChainDef.count = static_cast<int>(Primary.size());
				ChainDef.materials = &Material;
				ChainDef.materialCount = 1;
				ChainDef.isLoop = ShapeRef.bLoop;
				ChainDef.userData = Owner;
				ChainID = b2CreateChain(ID, &ChainDef);

				constexpr bool NOT_REVERSED = false;
				if (ShapeRef.bBlockBothSides) {
					std::vector<b2Vec2> Secondary = Utility::MakeBox2DChainPoints(ShapeRef.Points, ShapeRef.bLoop, NOT_REVERSED);
					b2ChainDef SecondaryDef = b2DefaultChainDef();
					SecondaryDef.points = Secondary.data();
					SecondaryDef.count = static_cast<int>(Secondary.size());
					SecondaryDef.materials = &Material;
					SecondaryDef.materialCount = 1;
					SecondaryDef.isLoop = ShapeRef.bLoop;
					SecondaryDef.userData = Owner;
					ChainID2 = b2CreateChain(ID, &SecondaryDef);
				}
				break;
			}
			case EShape::Line:
				LK_ASSERT(false);
				break;
			case EShape::None:
				LK_ASSERT(false);
				break;
		}
	}

	EBodyType CBody::DetermineBodyType(const b2BodyType Type)
	{
		switch (Type) {
			case b2BodyType::b2_staticBody:
				return EBodyType::Static;
			case b2BodyType::b2_dynamicBody:
				return EBodyType::Dynamic;
			case b2BodyType::b2_kinematicBody:
				return EBodyType::Kinematic;
			default:
				LK_VERIFY(false);
				return EBodyType::COUNT;
		}
	}

}
