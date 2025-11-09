#include "font.h"

#include <array>

namespace platformer2d::UI {

	namespace
	{
		using TFontArray = std::array<ImFont*, static_cast<std::size_t>(EFontSize::COUNT)>;
		using TFontMap = std::unordered_map<EFont, TFontArray>;

		std::unordered_map<EFontModifier, TFontMap> FontMap = {
			{ EFontModifier::Normal,     { { EFont::SourceSansPro, TFontArray{} } } },
			{ EFontModifier::Bold  ,     { { EFont::Roboto,        TFontArray{} } } },
			{ EFontModifier::Italic,     { { EFont::FontAwesome,   TFontArray{} } } },
			{ EFontModifier::BoldItalic, { { EFont::FontAwesome,   TFontArray{} } } },
			{ EFontModifier::SemiMedium, { { EFont::FontAwesome,   TFontArray{} } } },
		};
	}

	const std::unordered_map<EFontSize, float> FontSizeMap = {
		{ EFontSize::Regular, 22.0f },
		{ EFontSize::Small,   12.0f },
		{ EFontSize::Smaller, 16.0f },
		{ EFontSize::Large,   26.0f },
		{ EFontSize::Larger,  30.0f },
		{ EFontSize::Header,  38.0f },
		{ EFontSize::Title,   52.0f },
		{ EFontSize::Banner,  64.0f },
	};

	void Font::Add(const FFontConfiguration& FontConfig, const bool IsDefault)
	{
		LK_VERIFY((FontConfig.Font != EFont::None) && (FontConfig.Font != EFont::COUNT), "Invalid font");
		LK_VERIFY((FontConfig.Size != EFontSize::None) && (FontConfig.Size != EFontSize::COUNT), "Invalid font size");
		LK_VERIFY(std::filesystem::exists(FontConfig.FilePath), "Invalid font: {}", FontConfig.FilePath);

		ImGuiIO& IO = ImGui::GetIO();
		ImFontConfig ImguiFontConfig;
		ImguiFontConfig.MergeMode = FontConfig.MergeWithLast;
		ImFont* Font = IO.Fonts->AddFontFromFileTTF(
			FontConfig.FilePath.string().c_str(),
			FontSizeMap.at(FontConfig.Size),
			&ImguiFontConfig,
			(FontConfig.GlyphRanges == nullptr ? IO.Fonts->GetGlyphRangesDefault() : FontConfig.GlyphRanges)
		);
		LK_VERIFY(Font, "Failed to load font: {}", FontConfig.FilePath);

		TFontArray& ArrayRef = FontMap.at(FontConfig.Modifier)[FontConfig.Font];
		/* Verify the font hasn't been added yet. */
		if (FontConfig.Font != EFont::FontAwesome)
		{
			LK_VERIFY(ArrayRef[static_cast<int>(FontConfig.Size)] == nullptr,
					  "Font {} ({}) already added", Enum::ToString(FontConfig.Font), Enum::ToString(FontConfig.Size));
		}
		ArrayRef[static_cast<int>(FontConfig.Size)] = Font;
		LK_TRACE_TAG("Font", "Added: {} ({})", FontConfig.FilePath.filename(), FontSizeMap.at(FontConfig.Size));

		if (IsDefault)
		{
			IO.FontDefault = Font;
		}
	}

	void Font::Push(const EFont Font, const EFontSize Size, const EFontModifier Modifier)
	{
		ImFont* FontRef = FontMap.at(Modifier)[Font][static_cast<int>(Size)];
		LK_ASSERT(FontRef, "Invalid font: {} ({}), {}", Enum::ToString(Font), Enum::ToString(Size), Enum::ToString(Modifier));
		ImGui::PushFont(FontRef);
	}

	void Font::Pop()
	{
		ImGui::PopFont();
	}

	ImFont* Font::Get(EFont Font, EFontSize Size, EFontModifier Modifier)
	{
		ImFont* FontRef = FontMap.at(Modifier)[Font][static_cast<int>(Size)];
		return FontRef;
	}

}
