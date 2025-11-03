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

		template<typename T>
		struct TAlwaysFalse
		{
			static constexpr bool value = false;
		};

		template<typename T>
		constexpr bool TAlwaysFalse_v = TAlwaysFalse<T>::value;
	}

	template<EShape T>
	inline bool IsShape(const TShape& S)
	{
		LK_ASSERT(std::get_if<TShapeType<T>>(&S) != nullptr);
		return std::holds_alternative<TShapeType<T>>(S);
	}

	inline EShape DetermineShapeType(const TShape& S)
	{
		if (std::holds_alternative<FPolygon>(S)) return EShape::Polygon;
		else if (std::holds_alternative<FLine>(S)) return EShape::Line;
		else if (std::holds_alternative<FCapsule>(S)) return EShape::Capsule;
		else return EShape::None;
	}

	template<typename TShape>
	inline glm::vec2 GetBoundingBox(const TShape& S)
	{
		static_assert(_Internal::TAlwaysFalse_v<TShape>, "Specialization missing");
		return glm::vec2(0.0f, 0.0f);
	}

	template<>
	inline glm::vec2 GetBoundingBox<FPolygon>(const FPolygon& S)
	{
		const float C = std::cos(S.Rotation);
		const float Si = std::sin(S.Rotation);
		const float AbsC = std::abs(C);
		const float AbsS = std::abs(Si);

		const float Width = (AbsC * S.Size.x) + (AbsS * S.Size.y) + (2.0f * S.Radius);
		const float Height = (AbsS * S.Size.x) + (AbsC * S.Size.y) + (2.0f * S.Radius);

		return glm::vec2(Width, Height);
	}

	template<>
	inline glm::vec2 GetBoundingBox<FLine>(const FLine& S)
	{
		const glm::vec2 Delta = S.P1 - S.P0;
		return Delta;
	}

	template<>
	inline glm::vec2 GetBoundingBox<FCapsule>(const FCapsule& S)
	{
		const glm::vec2 Segment = S.P1 - S.P0;
		const glm::vec2 AbsSeg = glm::abs(Segment);
		return AbsSeg + glm::vec2(2.0f * S.Radius, 2.0f * S.Radius);
	}

}