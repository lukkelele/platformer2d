#pragma once

#include <variant>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

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
		float Rotation = glm::pi<float>(); /* Radians */
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

	template<EShape> struct TShapeMap;
	template<> struct TShapeMap<EShape::Polygon> { using Type = FPolygon; };
	template<> struct TShapeMap<EShape::Line>    { using Type = FLine; };
	template<> struct TShapeMap<EShape::Capsule> { using Type = FCapsule; };

	template<EShape S>
	using TShapeType = typename TShapeMap<S>::Type;

	namespace _Internal
	{
		template<typename T>
		concept IsShapeVariant = std::disjunction_v<
			std::is_same<T, FPolygon>,
			std::is_same<T, FLine>,
			std::is_same<T, FCapsule>
		>;
	}

	template<EShape T>
	bool IsShape(const TShape& S)
	{
		LK_ASSERT(std::get_if<TShapeType<T>>(&S) != nullptr);
		return std::holds_alternative<TShapeType<T>>(S);
	}

}