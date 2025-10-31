#pragma once

#include <string>
#include <string_view>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/core.h"
#include "fontawesome.h"

namespace platformer2d {

	enum class EFontSize
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

	enum class EFontModifier
	{
		Normal,
		Bold,
		Italic,
		BoldItalic,
		SemiMedium,
	};

	enum class EFont
	{
		None,
		SourceSansPro,
		Roboto,
		FontAwesome,
		COUNT
	};

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

	namespace UI::Font 
	{
		void Add(const FFontConfiguration& FontConfig, bool IsDefault = false);
		void Push(EFont Font, EFontSize Size = EFontSize::Regular, EFontModifier Modifier = EFontModifier::Normal);
		void Pop();
	}

	namespace Enum
	{
		static const char* ToString(const EFont Font)
		{
			switch (Font)
			{
				case EFont::None:          return "None";
				case EFont::SourceSansPro: return "SourceSansPro";
				case EFont::Roboto:        return "Roboto";
				case EFont::FontAwesome:   return "FontAwesome";
			}

			LK_VERIFY(false);
			return nullptr;
		}

		static const char* ToString(const EFontSize FontSize)
		{
			switch (FontSize)
			{
				case EFontSize::None:    return "None";
				case EFontSize::Smaller: return "Smaller";
				case EFontSize::Small:   return "Small";
				case EFontSize::Regular: return "Regular";
				case EFontSize::Large:   return "Large";
				case EFontSize::Larger:  return "Larger";
				case EFontSize::Header:  return "Header";
				case EFontSize::Title:   return "Title";
			}

			LK_VERIFY(false);
			return nullptr;
		}

		static const char* ToString(const EFontModifier FontModifier)
		{
			switch (FontModifier)
			{
				case EFontModifier::Normal:     return "Normal";
				case EFontModifier::Bold:       return "Bold";
				case EFontModifier::Italic:     return "Italic";
				case EFontModifier::BoldItalic: return "BoldItalic";
				case EFontModifier::SemiMedium: return "SemiMedium";
			}

			LK_VERIFY(false);
			return nullptr;
		}
	}
}

namespace std 
{
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
