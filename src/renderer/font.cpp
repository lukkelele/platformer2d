#include "font.h"

#include <array>

namespace platformer2d::UI {

	namespace {
		using TFontArray = std::array<ImFont*, static_cast<std::size_t>(EFontSize::COUNT)>;
		using TFontMatrix = std::array<
			std::array<TFontArray, static_cast<std::size_t>(EFont::COUNT)>,
			static_cast<std::size_t>(EFontModifier::COUNT)>;
	}

	static TFontMatrix FontMatrix{};

	static constexpr std::array<float, static_cast<std::size_t>(EFontSize::COUNT)> FontSizeMap = []() constexpr
	{
		std::array<float, static_cast<std::size_t>(EFontSize::COUNT)> Sizes{};
		/* clang-format off */
		Sizes[static_cast<std::size_t>(EFontSize::Regular)] = 22.0f;
		Sizes[static_cast<std::size_t>(EFontSize::Smaller)] = 12.0f;
		Sizes[static_cast<std::size_t>(EFontSize::Small)]   = 20.0f;
		Sizes[static_cast<std::size_t>(EFontSize::Large)]   = 26.0f;
		Sizes[static_cast<std::size_t>(EFontSize::Larger)]  = 30.0f;
		Sizes[static_cast<std::size_t>(EFontSize::Header)]  = 38.0f;
		Sizes[static_cast<std::size_t>(EFontSize::Title)]   = 58.0f;
		Sizes[static_cast<std::size_t>(EFontSize::Banner)]  = 82.0f;
		/* clang-format on */
		return Sizes;
	}();

	static ImFont*& GetFontRef(const EFont Font, const EFontSize Size, const EFontModifier Modifier)
	{
		LK_ASSERT((Font != EFont::None) && (Font != EFont::COUNT), "Invalid font: {}", Enum::ToString(Font));
		LK_ASSERT(Size != EFontSize::COUNT, "Invalid font size: {}", Enum::ToString(Size));
		LK_ASSERT(Modifier != EFontModifier::COUNT, "Invalid font modifier");
		return FontMatrix.at(static_cast<std::size_t>(Modifier))
			.at(static_cast<std::size_t>(Font))
			.at(static_cast<std::size_t>(Size));
	}

	void Font::Add(const FFontConfiguration& FontConfig, const bool IsDefault)
	{
		LK_VERIFY((FontConfig.Font != EFont::None) && (FontConfig.Font != EFont::COUNT), "Invalid font");
		LK_VERIFY(FontConfig.Size != EFontSize::COUNT, "Invalid font size");
		LK_VERIFY(std::filesystem::exists(FontConfig.FilePath), "Invalid font: {}", FontConfig.FilePath);

		const float SizePx = FontSizeMap[static_cast<std::size_t>(FontConfig.Size)];
		ImGuiIO& IO = ImGui::GetIO();
		ImFontConfig ImguiFontConfig;
		ImguiFontConfig.MergeMode = FontConfig.MergeWithLast;
		ImFont* FontPtr = IO.Fonts->AddFontFromFileTTF(
			FontConfig.FilePath.string().c_str(),
			SizePx,
			&ImguiFontConfig,
			(FontConfig.GlyphRanges == nullptr ? IO.Fonts->GetGlyphRangesDefault() : FontConfig.GlyphRanges));
		LK_VERIFY(FontPtr, "Failed to load font: {}", FontConfig.FilePath);

		ImFont*& Slot = GetFontRef(FontConfig.Font, FontConfig.Size, FontConfig.Modifier);
		/* Verify the font hasn't been added yet. */
		if (FontConfig.Font != EFont::FontAwesome) {
			LK_VERIFY(Slot == nullptr,
				"Font {} ({}) already added", Enum::ToString(FontConfig.Font), Enum::ToString(FontConfig.Size));
		}
		Slot = FontPtr;
		LK_TRACE_TAG("Font", "Added: {} ({})", FontConfig.FilePath.filename(), SizePx);

		if (IsDefault) {
			IO.FontDefault = FontPtr;
		}
	}

	void Font::Push(const EFont Font, const EFontSize Size, const EFontModifier Modifier)
	{
		ImGui::PushFont(GetFontRef(Font, Size, Modifier));
	}

	void Font::Push(const EFontSize Size, const EFontModifier Modifier)
	{
		ImGui::PushFont(GetFontRef(Font::GetDefault(), Size, Modifier));
	}

	void Font::Pop()
	{
		ImGui::PopFont();
	}

	ImFont* Font::Get(const EFont Font, const EFontSize Size, const EFontModifier Modifier)
	{
		return GetFontRef(Font, Size, Modifier);
	}

}
