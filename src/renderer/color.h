#pragma once

#include <glm/glm.hpp>
#include <imgui/imgui.h>

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
		using VecType = glm::vec3;
		using B = TColorInternal<Range>;
	public:
		static constexpr glm::vec3 White       = { B::Scale(1.0f), B::Scale(1.0f), B::Scale(1.0f) };
		static constexpr glm::vec3 Black       = { B::Scale(0.0f), B::Scale(0.0f), B::Scale(1.0f) };
		static constexpr glm::vec3 Red         = { B::Scale(1.0f), B::Scale(0.0f), B::Scale(1.0f) };
		static constexpr glm::vec3 Green       = { B::Scale(0.0f), B::Scale(1.0f), B::Scale(1.0f) };
		static constexpr glm::vec3 Blue        = { B::Scale(0.0f), B::Scale(0.0f), B::Scale(1.0f) };
		static constexpr glm::vec3 Transparent = { B::Scale(0.0f), B::Scale(0.0f), B::Scale(0.0f) };
	};

	template<EColorRange Range>
	struct TColor<glm::vec4, Range> : TColorInternal<Range>
	{
	private:
		using VecType = glm::vec4;
		using B = TColorInternal<Range>;
	public:
		static constexpr glm::vec4 White       = { B::Scale(1.0f),   B::Scale(1.0f),   B::Scale(1.0f),   B::Scale(1.0f) };
		static constexpr glm::vec4 Black       = { B::Scale(0.0f),   B::Scale(0.0f),   B::Scale(0.0f),   B::Scale(1.0f) };
		static constexpr glm::vec4 Red         = { B::Scale(1.0f),   B::Scale(0.0f),   B::Scale(0.0f),   B::Scale(1.0f) };
		static constexpr glm::vec4 Green       = { B::Scale(0.0f),   B::Scale(1.0f),   B::Scale(0.0f),   B::Scale(1.0f) };
		static constexpr glm::vec4 LightGreen  = { B::Scale(0.0f),   B::Scale(0.75f),  B::Scale(0.0f),   B::Scale(1.0f) };
		static constexpr glm::vec4 Blue        = { B::Scale(0.0f),   B::Scale(0.0f),   B::Scale(1.0f),   B::Scale(1.0f) };
		static constexpr glm::vec4 LightBlue   = { B::Scale(0.60f),  B::Scale(0.80f),  B::Scale(1.0f),   B::Scale(1.0f) };
		static constexpr glm::vec4 NiceBlue    = { B::Scale(0.325f), B::Scale(0.91f),  B::Scale(0.99f),  B::Scale(1.0f) };
		static constexpr glm::vec4 Gray        = { B::Scale(0.45f),  B::Scale(0.45f),  B::Scale(0.45f),  B::Scale(1.0f) };
		static constexpr glm::vec4 LightGray   = { B::Scale(0.70f),  B::Scale(0.70f),  B::Scale(0.70f),  B::Scale(1.0f) };
		static constexpr glm::vec4 Cyan        = { B::Scale(0.0f),   B::Scale(1.0f),   B::Scale(1.0f),   B::Scale(1.0f) };
		static constexpr glm::vec4 Transparent = { B::Scale(0.0f),   B::Scale(0.0f),   B::Scale(0.0f),   B::Scale(0.0f) };

		template<typename To = VecType, typename From>
		static inline constexpr To Convert(const From InColor);
	};

	template<EColorRange Range = EColorRange::Normalized>
	using TColor4 = TColor<glm::vec4, Range>;
	using FColor = TColor4<EColorRange::Normalized>;

	template<EColorRange Range = EColorRange::Normalized>
	using TColor3 = TColor<glm::vec3, Range>;
	using FColor3 = TColor3<EColorRange::Normalized>;

	template<>
	template<>
	inline glm::vec4 TColor<glm::vec4, EColorRange::Normalized>::Convert(const uint32_t InColor)
	{
		const float R = static_cast<float>((InColor >>  0) & 0xFF) / 255.0f;
		const float G = static_cast<float>((InColor >>  8) & 0xFF) / 255.0f;
		const float B = static_cast<float>((InColor >> 16) & 0xFF) / 255.0f;
		const float A = static_cast<float>((InColor >> 24) & 0xFF) / 255.0f;
		return glm::vec4(R, G, B, A);
	}

	template<>
	template<>
	inline uint32_t TColor<glm::vec4, EColorRange::Normalized>::Convert(const glm::vec4& InColor)
	{
		const uint32_t R = static_cast<uint32_t>(InColor.r * 255.0f);
		const uint32_t G = static_cast<uint32_t>(InColor.g * 255.0f);
		const uint32_t B = static_cast<uint32_t>(InColor.b * 255.0f);
		const uint32_t A = static_cast<uint32_t>(InColor.a * 255.0f);
		return (A << 24) | (B << 16) | (G << 8) | R;
	}

	template<>
	template<>
	inline ImVec4 TColor<glm::vec4, EColorRange::Normalized>::Convert(const glm::vec4& InColor)
	{
		return ImVec4(InColor.r, InColor.g, InColor.b, InColor.a);
	}

	template<>
	template<>
	inline ImVec4 TColor<glm::vec4, EColorRange::Normalized>::Convert(const glm::vec4 InColor)
	{
		return ImVec4(InColor.r, InColor.g, InColor.b, InColor.a);
	}

	template<>
	template<>
	inline ImVec4 TColor<glm::vec4, EColorRange::Normalized>::Convert(const uint32_t InColor)
	{
		const float R = static_cast<float>((InColor >>  0) & 0xFF) / 255.0f;
		const float G = static_cast<float>((InColor >>  8) & 0xFF) / 255.0f;
		const float B = static_cast<float>((InColor >> 16) & 0xFF) / 255.0f;
		const float A = static_cast<float>((InColor >> 24) & 0xFF) / 255.0f;
		return ImVec4(R, G, B, A);
	}

	namespace RGBA32 
	{
		inline constexpr uint32_t White          = IM_COL32(255, 255, 255, 255);
		inline constexpr uint32_t Black          = IM_COL32(0, 0, 0, 255);
		inline constexpr uint32_t Gray           = IM_COL32(128, 128, 128, 255);
		inline constexpr uint32_t LightGray      = IM_COL32(165, 165, 165, 255);
		inline constexpr uint32_t LighterGray    = IM_COL32(190, 190, 190, 255);
		inline constexpr uint32_t Red            = IM_COL32(255, 0, 0, 255);
		inline constexpr uint32_t Green          = IM_COL32(0, 255, 0, 255);
		inline constexpr uint32_t Blue           = IM_COL32(0, 0, 255, 255);
		inline constexpr uint32_t Cyan           = IM_COL32(0, 255, 255, 255);
		inline constexpr uint32_t Magenta        = IM_COL32(255, 0, 255, 255);
		inline constexpr uint32_t Yellow         = IM_COL32(255, 255, 0, 255);
		inline constexpr uint32_t Orange         = IM_COL32(255, 165, 0, 255);
		inline constexpr uint32_t Purple         = IM_COL32(128, 0, 128, 255);
		inline constexpr uint32_t Pink           = IM_COL32(255, 192, 203, 255);
		inline constexpr uint32_t Brown          = IM_COL32(165, 42, 42, 255);
		inline constexpr uint32_t DarkRed        = IM_COL32(139, 0, 0, 255);
		inline constexpr uint32_t DarkGreen      = IM_COL32(0, 100, 0, 255);
		inline constexpr uint32_t DarkBlue       = IM_COL32(0, 0, 139, 255);
		inline constexpr uint32_t DarkCyan       = IM_COL32(0, 139, 139, 255);
		inline constexpr uint32_t DarkMagenta    = IM_COL32(139, 0, 139, 255);
		inline constexpr uint32_t DarkYellow     = IM_COL32(204, 204, 0, 255);
		inline constexpr uint32_t BrightGreen    = IM_COL32(18, 140, 40, 255);
		inline constexpr uint32_t NiceBlue       = IM_COL32(83, 232, 254, 255);
		inline constexpr uint32_t NiceGreen      = IM_COL32(0, 205, 15, 192);

		inline constexpr uint32_t Accent         = IM_COL32(236, 158, 36, 255);
		inline constexpr uint32_t Highlight      = IM_COL32(39, 185, 242, 255);
		inline constexpr uint32_t Compliment     = IM_COL32(78, 151, 166, 255);
		inline constexpr uint32_t PropertyField  = IM_COL32(15, 15, 15, 255);
		inline constexpr uint32_t Muted          = IM_COL32(77, 77, 77, 255);

		inline constexpr uint32_t GroupHeader       = IM_COL32(43, 43, 43, 255);
		inline constexpr uint32_t Selection         = IM_COL32(190, 205, 119, 255);
		inline constexpr uint32_t SelectionMuted    = IM_COL32(237, 201, 142, 23);
		inline constexpr uint32_t Background        = IM_COL32(36, 36, 36, 255);
		inline constexpr uint32_t BackgroundDark    = IM_COL32(26, 26, 26, 255);
		inline constexpr uint32_t BackgroundDarker  = IM_COL32(16, 16, 16, 255);

		namespace Text
		{
			inline constexpr uint32_t Normal   = IM_COL32(190, 190, 190, 255);
			inline constexpr uint32_t Brighter = IM_COL32(204, 204, 204, 255);
			inline constexpr uint32_t Darker   = IM_COL32(128, 128, 128, 255);
			inline constexpr uint32_t Error    = IM_COL32(232, 55, 55, 255);
			inline constexpr uint32_t Disabled = IM_COL32(237, 201, 142, 95);
		}

		namespace Titlebar
		{
			inline constexpr uint32_t Default = IM_COL32(30, 30, 30, 255);
			inline constexpr uint32_t Orange  = IM_COL32(186, 66, 30, 255);
			inline constexpr uint32_t Green   = IM_COL32(22, 84, 29, 255);
			inline constexpr uint32_t Red     = IM_COL32(190, 32, 30, 255);
		}
	}

}