#pragma once

#include <string>
#include <string_view>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/core.h"
#include "fontawesome.h"

namespace platformer2d {

	enum class EFontSize : std::int8_t
	{
		None = -1,
		Regular,
		Smaller,
		Small,
		Large,
		Larger,
		Header,
		Title,
		Banner,
		COUNT
	};
	LK_ENUM_RANGE(EFontSize, EFontSize::None, EFontSize::COUNT);

	enum class EFontModifier : std::uint8_t
	{
		Normal,
		Bold,
		Italic,
		BoldItalic,
		SemiMedium,
		COUNT
	};
	LK_ENUM(EFontModifier);

	enum class EFont
	{
		None,
		SourceSansPro,
		Roboto,
		FontAwesome,
		COUNT
	};
	LK_ENUM(EFont);

	struct FFontConfiguration
	{
		EFont Font;
		EFontSize Size = EFontSize::Regular;
		EFontModifier Modifier = EFontModifier::Normal;
		std::filesystem::path FilePath{};
		bool MergeWithLast = false;
		const ImWchar* GlyphRanges = nullptr;
	};

	extern const std::unordered_map<EFontSize, float> FontSizeMap;

	namespace UI::Font {
		void Add(const FFontConfiguration& FontConfig, bool IsDefault = false);
		void Push(EFont Font, EFontSize Size = EFontSize::Regular, EFontModifier Modifier = EFontModifier::Normal);
		void Push(EFontSize Size, EFontModifier Modifier = EFontModifier::Normal);
		void Pop();
		[[nodiscard]] ImFont* Get(EFont Font, EFontSize Size = EFontSize::Regular, EFontModifier Modifier = EFontModifier::Normal);

		[[nodiscard]] constexpr EFont GetDefault()
		{
			return EFont::SourceSansPro;
		}
	}
}

namespace std {
	template<>
	struct hash<::platformer2d::EFontModifier>
	{
		std::size_t operator()(const ::platformer2d::EFontModifier Modifier) const noexcept
		{
			return static_cast<std::size_t>(Modifier);
		}
	};

	template<>
	struct hash<::platformer2d::EFont>
	{
		std::size_t operator()(const ::platformer2d::EFont Font) const noexcept
		{
			return static_cast<std::size_t>(Font);
		}
	};

}
