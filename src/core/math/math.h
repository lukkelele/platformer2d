#pragma once

#include <glm/glm.hpp>

#include "core/assert.h"
#include "shapes.h"

#include <box2d/math_functions.h>

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

	glm::vec3 ConvertScreenToWorld(const glm::vec3& Point, const glm::vec3& Center,
								   float Width, float Height, float Zoom);
	glm::vec3 ConvertWorldToScreen(const glm::vec3& Point, const glm::vec3& Center,
								   float Width, float Height, float Zoom);


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
