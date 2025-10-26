#pragma once

#include <variant>

#include <glm/glm.hpp>

#include "core/macros.h"
#include "physics/bodytype.h"

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

}