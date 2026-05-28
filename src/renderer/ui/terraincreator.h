#pragma once

#include "core/core.h"
#include "core/delegate.h"
#include "core/math/math.h"
#include "renderer/color.h"
#include "renderer/font.h"
#include "renderer/texture.h"
#include "renderer/ui/ui_core.h"
#include "renderer/ui/scoped.h"

namespace platformer2d {
	class CScene;
}

namespace platformer2d::UI {
	struct FTerrainCreator
	{
		bool bOpen = true;
		std::vector<glm::vec2> Points = {
			{ -1.0f, 0.0f},
			{-0.50f, 0.0f},
			{ 0.50f, 0.0f},
			{  1.0f, 0.0f},
		};
		glm::vec2 PreviewOrigin = {0.0f, 0.0f}; /* World-space position used for new chains and the preview. */
		bool bLoop = false;
		bool bBlockBothSides = false;
		EColor Color = EColor::White;
		ETexture Texture = ETexture::White;
		float TextureHeight = 0.20f;
		EDirection TextureSide = EDirection::None;
		float TextureOffset = 0.0f;
		bool bTextureTile = false;
		float TextureTileWidth = 0.0f;
		std::array<char, 64> NameBuf = {0};
		LUUID EditTarget = LUUID::Null;
		bool bHasEditTarget = false;
		bool bPreviewVisible = false;

		struct
		{
			bool bLastNodeState = false;
			bool bPreviewVisible = false;
		} Cache{.bPreviewVisible = bPreviewVisible};

		void ResetPoints()
		{
			Points = {
				{ -1.0f, 0.0f},
				{-0.50f, 0.0f},
				{ 0.50f, 0.0f},
				{  1.0f, 0.0f},
			};
		}

		void OnDeselect()
		{
			ResetPoints();
			EditTarget = LUUID::Null;
			bHasEditTarget = false;
			PreviewOrigin = {0.0f, 0.0f};
			Texture = ETexture::White;
			TextureHeight = 0.20f;
			TextureSide = EDirection::None;
			TextureOffset = 0.0f;
			bTextureTile = false;
			TextureTileWidth = 0.0f;
		}
	};

	extern FTerrainCreator TerrainCreator;

	void RenderTerrainCreator(std::shared_ptr<CScene> Scene);
	void RenderChainPreview(const std::shared_ptr<CScene>& Scene);

}

