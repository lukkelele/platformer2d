#pragma once

#include <glm/glm.hpp>

#include "core/core.h"

namespace platformer2d {

	class CTexture;

	struct FSpriteUV
	{
		float U0 = 0.0f;
		float V0 = 0.0f;
		float U1 = 0.0f;
		float V1 = 0.0f;
	};

	class CSprite
	{
	public:
		CSprite(std::shared_ptr<CTexture> InTexture, const glm::vec2& InTilePos,
				const glm::vec2& InTileSize, bool InVerticalFlip = false);
		CSprite() = delete;
		~CSprite();

		const FSpriteUV& GetUV() const { return UV; }
		std::shared_ptr<CTexture> GetTexture() const { return Texture; }

		const glm::vec2& GetSize() const { return Size; }
		float GetWidth() const { return Size.x; }
		float GetHeight() const { return Size.y; }
		const glm::vec2& GetTilePos() const { return TilePos; }
		const glm::vec2& GetTileSize() const { return TileSize; }

		void SetTilePos(uint16_t X, uint16_t Y);
		void SetTilePos(const glm::vec2& InTilePos);
		std::size_t IncrementTilePosX(std::size_t Times = 1);
		std::size_t DecrementTilePosX(std::size_t Times = 1);

		[[nodiscard]] static FSpriteUV CalculateUV(const glm::vec2& InTilePos, const glm::vec2& InTileSize,
												   const glm::vec2& InSheetSize, bool InVerticalFlip = false);
		static void CalculateUV(FSpriteUV& InSpriteUV, const glm::vec2& InTilePos,
								const glm::vec2& InTileSize, const glm::vec2& InSheetSize, bool InVerticalFlip = false);

	private:
		std::shared_ptr<CTexture> Texture = nullptr;
		FSpriteUV UV;
		glm::vec2 Size;
		glm::vec2 TilePos;
		glm::vec2 TileSize;
		bool bVerticalFlip;
	};

}
