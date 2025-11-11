#pragma once

#include <random>

#include <glm/glm.hpp>
#include <box2d/math_functions.h>

#include "core/assert.h"
#include "shapes.h"

namespace platformer2d::Math {

	namespace _Internal
	{
		template<typename T>
		concept IsGlmVec = std::disjunction_v<
			std::is_same<T, glm::vec2>,
			std::is_same<T, glm::vec3>,
			std::is_same<T, glm::vec4>
		>;
	}

	inline glm::vec2 Perp(const glm::vec2& V)
	{
		return glm::vec2(-V.y, V.x);
	}

	float Randomize(float Min, float Max);

	glm::vec3 ConvertScreenToWorld(const glm::vec3& Point, const glm::vec3& Center,
								   float Width, float Height, float Zoom);
	glm::vec3 ConvertWorldToScreen(const glm::vec3& Point, const glm::vec3& Center,
								   float Width, float Height, float Zoom);

	inline float Normalize(const float Value, const float RangeMin, const float RangeMax)
	{
		return (Value - RangeMin) / (RangeMax - RangeMin);
	}

	inline glm::vec2 Normalize(const glm::vec2& Coord, const glm::vec2& RangeMin, const glm::vec2& RangeMax)
	{
		return glm::vec2 (
			(Coord.x - RangeMin.x) / (RangeMax.x - RangeMin.x),
			(Coord.y - RangeMin.y) / (RangeMax.y - RangeMin.y)
		);
	}

	inline glm::vec2 ToScreen(const glm::vec2& Normalized, const glm::vec2& ViewportSize)
	{
		return Normalized * ViewportSize;
	}

	inline glm::vec2 ToNdc(const glm::vec2& Normalized)
	{
		return glm::vec2(
			Normalized.x * (2.0f - 1.0f),
			Normalized.y * (2.0f - 1.0f)
		);
	}

	template<typename TVector>
	requires _Internal::IsGlmVec<TVector>
	TVector Convert(const b2Vec2& V)
	{
		if constexpr (std::is_same_v<TVector, glm::vec2>)
		{
			return glm::vec2(V.x, V.y);
		}
		else if constexpr (std::is_same_v<TVector, glm::vec3>)
		{
			return glm::vec3(V.x, V.y, 0.0f);
		}
		else if constexpr (std::is_same_v<TVector, glm::vec4>)
		{
			return glm::vec4(V.x, V.y, 0.0f, 0.0f);
		}
	}

	template<typename TVector>
	requires _Internal::IsGlmVec<TVector>
	b2Vec2 Convert(const TVector& V)
	{
		if constexpr (std::is_same_v<TVector, glm::vec2>)
		{
			return b2Vec2(V.x, V.y);
		}
		else if constexpr (std::is_same_v<TVector, glm::vec3>)
		{
			return b2Vec2(V.x, V.y);
		}
		else if constexpr (std::is_same_v<TVector, glm::vec4>)
		{
			return b2Vec2(V.x, V.y);
		}
	}

	/**
	 * @brief Copy values from source to destination.
	 */
	template<typename TVector>
	requires _Internal::IsGlmVec<TVector>
	void Convert(TVector& Dst, const b2Vec2& Src)
	{
		Dst.x = Src.x;
		Dst.y = Src.y;
	}

	/**
	 * @brief Copy values from source to destination.
	 */
	template<typename TVector>
	requires _Internal::IsGlmVec<TVector>
	void Convert(b2Vec2& Dst, const TVector& Src)
	{
		Dst.x = Src.x;
		Dst.y = Src.y;
	}

}


/************************************
 * Log formatters.
 ************************************/
template<>
struct std::formatter<b2Vec2>
{
	template<typename ParseContext>
    constexpr auto parse(ParseContext& Context)
    {
        return Context.begin();
    }

	template<typename FormatContext>
    auto format(const b2Vec2& Input, FormatContext& Context) const
    {
		return std::format_to(Context.out(), "({:.2f}, {:.2f})", Input.x, Input.y);
    }
};
