#pragma once

#include <glm/glm.hpp>

#include "core/core.h"
#include "texture.h"

namespace platformer2d {
	struct FGlyph
	{
		glm::vec2 PlaneMin = {0.0f, 0.0f};
		glm::vec2 PlaneMax = {0.0f, 0.0f};
		glm::vec2 AtlasMin = {0.0f, 0.0f};
		glm::vec2 AtlasMax = {0.0f, 0.0f};
		float Advance = 0.0f;
		bool bVisible = false;
	};

	struct FFontMetrics
	{
		float EmSize = 1.0f;
		float LineHeight = 1.2f;
		float Ascender = 1.0f;
		float Descender = 0.0f;
		float UnderlineY = 0.0f;
		float UnderlineThickness = 0.0f;
	};

	class CFontAtlas
	{
	public:
		CFontAtlas(const std::filesystem::path& JsonPath, const std::filesystem::path& AtlasPngPath);
		CFontAtlas() = delete;
		CFontAtlas(const CFontAtlas&) = delete;
		CFontAtlas(CFontAtlas&&) = delete;
		~CFontAtlas() = default;

		CFontAtlas& operator=(const CFontAtlas&) = delete;
		CFontAtlas& operator=(CFontAtlas&&) = delete;

		[[nodiscard]] const FGlyph& GetGlyph(std::uint32_t Codepoint) const;
		[[nodiscard]] const FFontMetrics& GetMetrics() const { return Metrics; }
		[[nodiscard]] float GetDistanceRange() const { return DistanceRange; }
		[[nodiscard]] const std::shared_ptr<CTexture>& GetTexture() const { return AtlasTexture; }

		[[nodiscard]] float MeasureWidth(std::string_view Text) const;

	private:
		void ParseJson(const std::filesystem::path& JsonPath);

	public:
		static constexpr std::uint32_t FIRST_CODEPOINT = 0x20;
		static constexpr std::uint32_t LAST_CODEPOINT = 0x7E;
		static constexpr std::size_t GLYPH_COUNT = (LAST_CODEPOINT - FIRST_CODEPOINT) + 1;

	private:
		std::shared_ptr<CTexture> AtlasTexture;
		std::array<FGlyph, GLYPH_COUNT> Glyphs{};
		FFontMetrics Metrics{};
		float DistanceRange = 4.0f;
		std::uint32_t AtlasWidth = 0;
		std::uint32_t AtlasHeight = 0;
	};

}
