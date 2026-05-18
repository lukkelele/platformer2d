#include "fontatlas.h"

#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>

#include "core/assert.h"
#include "core/log.h"

namespace platformer2d {

	CFontAtlas::CFontAtlas(const std::filesystem::path& JsonPath, const std::filesystem::path& AtlasPngPath)
	{
		LK_ASSERT(std::filesystem::exists(JsonPath), "Font JSON not found: {}", JsonPath);
		LK_ASSERT(std::filesystem::exists(AtlasPngPath), "Font atlas PNG not found: {}", AtlasPngPath);
		ParseJson(JsonPath);

		FTextureSpecification Spec = {
			.Path = AtlasPngPath,
			.Name = AtlasPngPath.stem().string(),
			.bFlipVertical = true,
			.Format = EImageFormat::RGBA8,
			.SamplerWrap = ETextureWrap::Clamp,
			.SamplerFilter = ETextureFilter::Linear,
		};
		AtlasTexture = std::make_shared<CTexture>(Spec);
		LK_TRACE_TAG("FontAtlas", "Loaded {} ({}x{}, PxRange={})", AtlasPngPath.filename().string(), AtlasWidth, AtlasHeight, DistanceRange);
	}

	const FGlyph& CFontAtlas::GetGlyph(const std::uint32_t Codepoint) const
	{
		if ((Codepoint < FIRST_CODEPOINT) || (Codepoint > LAST_CODEPOINT)) {
			return Glyphs.at(0);
		}
		return Glyphs.at(Codepoint - FIRST_CODEPOINT);
	}

	float CFontAtlas::MeasureWidth(const std::string_view Text) const
	{
		float Width = 0.0f;
		for (std::size_t Idx = 0; Idx < Text.size(); Idx++) {
			const std::uint32_t Codepoint = static_cast<std::uint8_t>(Text[Idx]);
			if ((Codepoint < FIRST_CODEPOINT) || (Codepoint > LAST_CODEPOINT)) {
				continue;
			}
			Width += Glyphs.at(Codepoint - FIRST_CODEPOINT).Advance;
		}
		return Width;
	}

	void CFontAtlas::ParseJson(const std::filesystem::path& JsonPath)
	{
		std::ifstream Stream(JsonPath);
		LK_ASSERT(Stream.is_open(), "Failed to open font JSON: {}", JsonPath);

		std::stringstream Buf;
		Buf << Stream.rdbuf();
		const std::string Contents = Buf.str();

		using nlohmann::json;
		json Root = json::parse(Contents, nullptr, false);
		LK_ASSERT(!Root.is_discarded(), "Invalid font JSON: {}", JsonPath);

		LK_ASSERT(Root.contains("atlas") && Root.contains("glyphs"), "Malformed font JSON: {}", JsonPath);
		const json& Atlas = Root["atlas"];
		AtlasWidth = Atlas.value("width", 0);
		AtlasHeight = Atlas.value("height", 0);
		DistanceRange = Atlas.value("distanceRange", 4.0f);
		LK_ASSERT((AtlasWidth > 0) && (AtlasHeight > 0), "Atlas dimensions invalid in {}", JsonPath);

		if (Root.contains("metrics")) {
			const json& M = Root["metrics"];
			Metrics.EmSize = M.value("emSize", 1.0f);
			Metrics.LineHeight = M.value("lineHeight", 1.2f);
			Metrics.Ascender = M.value("ascender", 1.0f);
			Metrics.Descender = M.value("descender", 0.0f);
			Metrics.UnderlineY = M.value("underlineY", 0.0f);
			Metrics.UnderlineThickness = M.value("underlineThickness", 0.0f);
		}

		const float AtlasW = static_cast<float>(AtlasWidth);
		const float AtlasH = static_cast<float>(AtlasHeight);

		const json& GlyphArray = Root["glyphs"];
		for (const json& Entry : GlyphArray) {
			const std::uint32_t Codepoint = Entry.value("unicode", 0);
			if ((Codepoint < FIRST_CODEPOINT) || (Codepoint > LAST_CODEPOINT)) {
				continue;
			}
			FGlyph& G = Glyphs[Codepoint - FIRST_CODEPOINT];
			G.Advance = Entry.value("advance", 0.0f);

			if (Entry.contains("planeBounds") && Entry.contains("atlasBounds")) {
				const json& P = Entry["planeBounds"];
				const json& A = Entry["atlasBounds"];
				G.PlaneMin = {P.value("left", 0.0f), P.value("bottom", 0.0f)};
				G.PlaneMax = {P.value("right", 0.0f), P.value("top", 0.0f)};
				G.AtlasMin = {A.value("left", 0.0f) / AtlasW, A.value("bottom", 0.0f) / AtlasH};
				G.AtlasMax = {A.value("right", 0.0f) / AtlasW, A.value("top", 0.0f) / AtlasH};
				G.bVisible = true;
			}
		}
	}

}

