#pragma once

#include <variant>

#include "core/core.h"
#include "physics/bodytype.h"

#include <box2d/types.h>

namespace platformer2d {

	enum class EShape
	{
		None    = 0,
		Polygon = LK_BIT(1),
		Line    = LK_BIT(2),
		Capsule = LK_BIT(3),
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

	struct FLine
	{
		glm::vec2 P0 = { 0.0f, 0.0f };
		glm::vec2 P1 = { 0.0f, 0.0f };
	};

	using TShape = std::variant<std::monostate, FPolygon, FCapsule, FLine>;

	struct FActorSpecification
	{
		std::string Name = "Unnamed";
		b2BodyDef BodyDef;
		b2ShapeDef ShapeDef;

		TShape Shape{};

		FActorSpecification()
		{
			BodyDef = b2DefaultBodyDef();
			ShapeDef = b2DefaultShapeDef();
		}
		FActorSpecification(const FActorSpecification&) = default;
	};

}