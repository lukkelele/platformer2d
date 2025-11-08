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

	struct FSpriteAnimation
	{
		uint16_t StartTileX = 0;
		uint16_t StartTileY = 0;
		std::size_t FrameCount = 0;
		uint16_t TicksPerFrame = 4;

		inline uint16_t CalculateAnimFrame(const uint16_t FrameIndex) const
		{
			return (FrameIndex / TicksPerFrame) % FrameCount;
		}
	};

	class CSprite
	{
	public:
		CSprite(std::shared_ptr<CTexture> InTexture, const glm::vec2& InTilePos,
				const glm::vec2& InTileSize, bool FlipHorizontal = false, bool FlipVertical = false);
		CSprite() = delete;
		~CSprite();

		const FSpriteUV& GetUV() const { return UV; }
		std::shared_ptr<CTexture> GetTexture() const { return Texture; }

		const glm::vec2& GetSize() const { return Size; }
		float GetWidth() const { return Size.x; }
		float GetHeight() const { return Size.y; }
		const glm::vec2& GetTilePos() const { return TilePos; }
		uint16_t GetTilePosX() const { return TilePos.x; }
		uint16_t GetTilePosY() const { return TilePos.y; }
		const glm::vec2& GetTileSize() const { return TileSize; }

		void SetTilePos(uint16_t X, uint16_t Y, bool FlipHorizontal = false, bool FlipVertical = false);
		void SetTilePos(const glm::vec2& InTilePos, bool FlipHorizontal = false, bool FlipVertical = false);
		uint16_t IncrementTilePosX(std::size_t Times = 1);
		uint16_t DecrementTilePosX(std::size_t Times = 1);

		void FlipHorizontal();
		void FlipVertical();

		[[nodiscard]] static FSpriteUV CalculateUV(const glm::vec2& InTilePos, const glm::vec2& InTileSize,
												   const glm::vec2& InSheetSize, bool FlipHorizontal = false, bool FlipVertical = false);
		static void CalculateUV(FSpriteUV& InSpriteUV, const glm::vec2& InTilePos, const glm::vec2& InTileSize,
								const glm::vec2& InSheetSize, bool FlipHorizontal = false, bool FlipVertical = false);

	private:
		void UpdateSprite(bool FlipHorizontal = false, bool FlipVertical = false);

	private:
		std::shared_ptr<CTexture> Texture = nullptr;
		FSpriteUV UV;
		glm::vec2 Size;
		glm::vec2 TilePos;
		glm::vec2 TileSize;
	};

}
