#pragma once

#include <vector>

#include <glm/glm.hpp>

#include "core/core.h"

namespace platformer2d {

	class CTexture;

	struct FSpriteCoord
	{
		std::uint16_t X = 0;
		std::uint16_t Y = 0;

		bool operator==(const FSpriteCoord&) const = default;
	};

	struct FSpriteFrame
	{
		FSpriteCoord Current;
		FSpriteCoord Next;
	};

	struct FSpriteUV
	{
		float U0 = 0.0f;
		float V0 = 0.0f;
		float U1 = 0.0f;
		float V1 = 0.0f;
	};

	struct FSpriteAnimation
	{
		std::vector<FSpriteCoord> Frames;
		uint16_t StartTileX = 0;
		uint16_t StartTileY = 0;
		std::size_t FrameCount = 0;
		uint16_t TicksPerFrame = 1;

		[[nodiscard]] FSpriteCoord GetFrame(const std::uint16_t FrameIndex) const
		{
			if (!Frames.empty()) {
				return Frames[(FrameIndex / TicksPerFrame) % Frames.size()];
			}
			const std::uint16_t X = StartTileX + ((FrameIndex / TicksPerFrame) % FrameCount);
			return {X, StartTileY};
		}

		[[nodiscard]] std::uint16_t CalculateAnimFrame(const std::uint16_t FrameIndex) const
		{
			return StartTileX + ((FrameIndex / TicksPerFrame) % FrameCount);
		}

		[[nodiscard]] std::size_t Count() const
		{
			return !Frames.empty() ? Frames.size() : FrameCount;
		}

		[[nodiscard]] FSpriteCoord First() const
		{
			return !Frames.empty() ? Frames.front() : FSpriteCoord{StartTileX, StartTileY};
		}
	};

	class CSprite
	{
	public:
		CSprite(std::shared_ptr<CTexture> InTexture, const glm::vec2& InTilePos,
			const glm::vec2& InTileSize, const FSpriteAnimation& InAnim = FSpriteAnimation(),
			bool FlipHorizontal = false, bool FlipVertical = false);
		CSprite() = delete;
		~CSprite();

		[[nodiscard]] const FSpriteUV& GetUV() const { return UV; }
		[[nodiscard]] std::shared_ptr<CTexture> GetTexture() const { return Texture; }

		[[nodiscard]] const glm::vec2& GetSize() const { return Size; }
		[[nodiscard]] float GetWidth() const { return Size.x; }
		[[nodiscard]] float GetHeight() const { return Size.y; }
		[[nodiscard]] const glm::vec2& GetTilePos() const { return TilePos; }
		[[nodiscard]] std::uint16_t GetTilePosX() const { return TilePos.x; }
		[[nodiscard]] std::uint16_t GetTilePosY() const { return TilePos.y; }
		[[nodiscard]] const glm::vec2& GetTileSize() const { return TileSize; }
		[[nodiscard]] const FSpriteAnimation& GetAnimation() const { return Anim; }

		void SetTilePos(std::uint16_t X, std::uint16_t Y, bool FlipHorizontal = false, bool FlipVertical = false);
		void SetTilePos(const glm::vec2& InTilePos, bool FlipHorizontal = false, bool FlipVertical = false);
		std::uint16_t IncrementTilePosX(std::size_t Times = 1);
		std::uint16_t DecrementTilePosX(std::size_t Times = 1);

		void FlipHorizontal();
		void FlipVertical();

		[[nodiscard]] static FSpriteUV CalculateUV(const glm::vec2& InTilePos, const glm::vec2& InTileSize,
			const glm::vec2& InSheetSize, bool FlipHorizontal = false, bool FlipVertical = false);
		static void CalculateUV(FSpriteUV& InSpriteUV, const glm::vec2& InTilePos, const glm::vec2& InTileSize,
			const glm::vec2& InSheetSize, bool FlipHorizontal = false, bool FlipVertical = false);

	private:
		void UpdateSprite(bool FlipHorizontal = false, bool FlipVertical = false);

	private:
		std::shared_ptr<CTexture> Texture;
		FSpriteUV UV;
		glm::vec2 Size;
		glm::vec2 TilePos;
		glm::vec2 TileSize;
		FSpriteAnimation Anim;
	};

}
