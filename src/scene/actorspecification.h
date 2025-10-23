#pragma once

#include <variant>

#include "core/core.h"
#include "physics/bodytype.h"

#include <box2d/types.h>

namespace platformer2d {

	enum EMotionLock
	{
		EMotionLock_None = 0,
		EMotionLock_X = LK_BIT(1),
		EMotionLock_Y = LK_BIT(2),
		EMotionLock_Z = LK_BIT(3),
	};

	enum EShape
	{
		EShape_None = 0,
		EShape_Polygon = LK_BIT(1),
		EShape_Capsule = LK_BIT(2),
		EShape_Segment = LK_BIT(3),
	};

	struct FPolygon
	{
		glm::vec2 Size = { 1.0f, 1.0f };
		float Radius = 0.50f;
		float Rotation = 0.50f;
	};

	struct FCapsule
	{
		glm::vec2 P0 = { 0.0f, 0.0f };
		glm::vec2 P1 = { 0.0f, 0.0f };
		float Radius = 0.50f;
	};

	struct FActorSpecification
	{
		std::string Name = "Unnamed";

		b2BodyDef BodyDef;
		b2ShapeDef ShapeDef;

		using TShape = std::variant<std::monostate, FPolygon, FCapsule, b2Segment>;
		TShape Shape{};
#if 0

		union {
			std::nullptr_t None;
			b2Polygon Polygon;
			struct FCapsule 
			{
				b2Capsule Object;
				float Radius;
			} Capsule;
			b2Segment Segment;
		} Shape;
#endif
		EShape ShapeType = EShape_None;

		FActorSpecification()
		{
			BodyDef = b2DefaultBodyDef();
			ShapeDef = b2DefaultShapeDef();
		}
		FActorSpecification(const FActorSpecification&) = default;
	};

}