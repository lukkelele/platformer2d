#pragma once

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

	struct FActorSpecification
	{
		std::string Name = "Unnamed";
		glm::vec2 Position = { 0.0f, 0.0f }; /* @todo: remove this */
		glm::vec2 Size = { 0.50f, 0.50f }; /* @todo remove this */

		/** @todo: Use std::variant */
		b2BodyDef BodyDef;
		b2ShapeDef ShapeDef;
		union {
			std::nullptr_t None;

			b2Polygon Polygon;

			struct FCapsule {
				b2Capsule Object;
				float Radius;
			} Capsule;

			b2Segment Segment;
		} Shape;
		EShape ShapeType = EShape_None;

		FActorSpecification()
		{
			BodyDef = b2DefaultBodyDef();
			ShapeDef = b2DefaultShapeDef();
		}
		FActorSpecification(const FActorSpecification&) = default;
	};

}