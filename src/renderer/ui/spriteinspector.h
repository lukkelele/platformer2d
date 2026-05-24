#pragma once

#include <array>
#include <filesystem>

#include <glm/glm.hpp>

#include "core/core.h"
#include "core/enum.h"
#include "game/spritereader.h"
#include "renderer/sprite.h"
#include "renderer/texture.h"
#include "renderer/texture_enums.h"

namespace platformer2d::UI {

	enum class EAssetSpriteSource : std::uint8_t
	{
		Enum,
		AdHoc,
		COUNT
	};
	LK_ENUM(EAssetSpriteSource);

	struct FSpriteInspectorAnim
	{
		ESpriteFrame Frame = ESpriteFrame::Idle;
		std::vector<std::uint16_t> Indices;
		std::uint16_t TicksPerFrame = 8;
		std::uint16_t Row = 0;
	};

	struct FSpriteInspector
	{
		bool bWindowOpen = false;

		EAssetSpriteSource Source = EAssetSpriteSource::Enum;
		ETexture EnumTexture = ETexture::Player;
		std::array<char, 64> SheetName{};
		ESpriteSheetType SheetType = ESpriteSheetType::Generic;
		glm::vec2 TileSize{32.0f, 32.0f};
		std::uint16_t DefaultRow = 0;
		std::vector<FSpriteInspectorAnim> Animations;
		std::size_t EditAnimIdx = 0;
		std::size_t PreviewAnimIdx = 0;
		float PreviewTimeAccum = 0.0f;
		float SimulatedTicksPerSec = 60.0f;
		float SheetZoom = 2.0f;
		float PreviewZoom = 8.0f;
		float LeftColWidth = 1200.0f;
		float SheetHeight = 380.0f;
		bool bAddDirectlyToAnim = true;
	};

	bool IsSpriteInspectorOpen();
	void OpenSpriteInspector();
	void CloseSpriteInspector();
	void ToggleSpriteInspector();
	void RenderSpriteInspector();
}

