#pragma once

#include <glm/glm.hpp>

namespace platformer2d {

	enum class EColorRange
	{
		Normalized, /* 0.0f <-> 1.0f */
		Byte,       /* 0.0f <-> 255.0f */
	};

	template<EColorRange Range>
	struct TColorInternal
	{
		static constexpr float Scale(const float Value)
		{
			switch (Range)
			{
				case EColorRange::Normalized: return Value;
				case EColorRange::Byte:       return (Value * 255.0f);
				default:                      return Value;
			}
		}
	};

	template<typename TVector = glm::vec4, EColorRange Range = EColorRange::Normalized>
	struct TColor;

	template<EColorRange Range>
	struct TColor<glm::vec3, Range> : TColorInternal<Range>
	{
	private:
		using B = TColorInternal<Range>;
	public:
		static inline glm::vec3 White       = { B::Scale(1.0f), B::Scale(1.0f), B::Scale(1.0f) };
		static inline glm::vec3 Black       = { B::Scale(0.0f), B::Scale(0.0f), B::Scale(1.0f) };
		static inline glm::vec3 Red         = { B::Scale(1.0f), B::Scale(0.0f), B::Scale(1.0f) };
		static inline glm::vec3 Green       = { B::Scale(0.0f), B::Scale(1.0f), B::Scale(1.0f) };
		static inline glm::vec3 Blue        = { B::Scale(0.0f), B::Scale(0.0f), B::Scale(1.0f) };
		static inline glm::vec3 Transparent = { B::Scale(0.0f), B::Scale(0.0f), B::Scale(0.0f) };
	};

	template<EColorRange Range>
	struct TColor<glm::vec4, Range> : TColorInternal<Range>
	{
	private:
		using B = TColorInternal<Range>;
	public:
		static inline glm::vec4 White       = { B::Scale(1.0f), B::Scale(1.0f), B::Scale(1.0f), B::Scale(1.0f) };
		static inline glm::vec4 Black       = { B::Scale(0.0f), B::Scale(0.0f), B::Scale(0.0f), B::Scale(1.0f) };
		static inline glm::vec4 Red         = { B::Scale(1.0f), B::Scale(0.0f), B::Scale(0.0f), B::Scale(1.0f) };
		static inline glm::vec4 Green       = { B::Scale(0.0f), B::Scale(1.0f), B::Scale(0.0f), B::Scale(1.0f) };
		static inline glm::vec4 Blue        = { B::Scale(0.0f), B::Scale(0.0f), B::Scale(1.0f), B::Scale(1.0f) };
		static inline glm::vec4 Transparent = { B::Scale(0.0f), B::Scale(0.0f), B::Scale(0.0f), B::Scale(0.0f) };
	};

	template<EColorRange Range = EColorRange::Normalized>
	using TColor4 = TColor<glm::vec4, Range>;
	using FColor = TColor4<EColorRange::Normalized>;

	template<EColorRange Range = EColorRange::Normalized>
	using TColor3 = TColor<glm::vec3, Range>;
	using FColor3 = TColor3<EColorRange::Normalized>;

}