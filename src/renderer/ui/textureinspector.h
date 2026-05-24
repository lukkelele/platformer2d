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

	struct FTextureInspector
	{
		bool bWindowOpen = false;
		std::array<char, 512> PathBuf{};
		FTextureSpecification Spec{};
		std::shared_ptr<CTexture> Texture;
		std::string Status;
		bool bDirty = false;
	};

#if 0
	struct FAssetInspectorSpritePane
	{
		EAssetSpriteSource Source = EAssetSpriteSource::Enum;
		ETexture EnumTexture = ETexture::Player;
		std::array<char, 64> SheetName{};
		ESpriteSheetType SheetType = ESpriteSheetType::Generic;
		glm::vec2 TileSize{32.0f, 32.0f};
		std::uint16_t DefaultRow = 0;
		std::vector<FAssetInspectorAnim> Animations;
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

	struct FAssetInspector
	{
		bool bTextureOpen = false;
		//bool bSpriteOpen = false;
		FAssetInspectorTexturePane Texture;
		//FAssetInspectorSpritePane Sprite;
	};

	extern FAssetInspector AssetInspector;
#endif

	bool IsTextureInspectorOpen();
	void OpenTextureInspector();
	void CloseTextureInspector();
	void ToggleTextureInspector();
	void RenderTextureInspector();
}
