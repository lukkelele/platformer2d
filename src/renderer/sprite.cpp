#include "sprite.h"

#include "texture.h"

namespace platformer2d {

	CSprite::CSprite(std::shared_ptr<CTexture> InTexture, const glm::vec2& InTilePos,
					 const glm::vec2& InTileSize, const bool FlipHorizontal, const bool FlipVertical)
		: Texture(InTexture)
		, Size(InTexture->GetWidth(), Texture->GetHeight())
		, TilePos(InTilePos)
		, TileSize(InTileSize)
	{
		LK_VERIFY(Texture && (Size.x > 0.0f) && (Size.y > 0.0f)
				  && (TilePos.x > 0.0f) && (TilePos.y > 0.0f)
				  && (TileSize.x > 0.0f) && (TileSize.y > 0.0f));
		LK_DEBUG_TAG("Sprite", "Created: {} Size=({}, {}) TilePos=({}, {}) TileSize=({}, {})", InTexture->GetFilePath().filename(),
					 Size.x, Size.y, TilePos.x, TilePos.y, TileSize.x, TileSize.y);

		UV = CalculateUV(TilePos, TileSize, Size, FlipHorizontal, FlipVertical);
	}

	CSprite::~CSprite()
	{
		if (Texture)
		{
			Texture.reset();
		}
	}

	void CSprite::SetTilePos(const uint16_t X, const uint16_t Y, const bool FlipHorizontal, const bool FlipVertical)
	{
		SetTilePos({ X, Y }, FlipHorizontal, FlipVertical);
	}

	void CSprite::SetTilePos(const glm::vec2& InTilePos, const bool FlipHorizontal, const bool FlipVertical)
	{
		if (TilePos != InTilePos)
		{
			TilePos = InTilePos;
			UpdateSprite(FlipHorizontal, FlipVertical);
		}
	}

	uint16_t CSprite::IncrementTilePosX(const std::size_t Times)
	{
		LK_ASSERT(Times > 0);
		TilePos.x += Times;
		UpdateSprite();
		return TilePos.x;
	}

	uint16_t CSprite::DecrementTilePosX(const std::size_t Times)
	{
		LK_ASSERT(Times > 0);
		TilePos.x -= Times;
		UpdateSprite();
		return TilePos.x;
	}

	void CSprite::FlipHorizontal()
	{
		std::swap(UV.U0, UV.U1);
	}

	void CSprite::FlipVertical()
	{
		std::swap(UV.V0, UV.V1);
	}

	FSpriteUV CSprite::CalculateUV(const glm::vec2& TilePos, const glm::vec2& TileSize,
								   const glm::vec2& SheetSize, const bool FlipHorizontal, const bool FlipVertical)
	{
		float U0 = (TilePos.x * TileSize.x)  / static_cast<float>(SheetSize.x);
		float U1 = ((TilePos.x + 1) * TileSize.x) / static_cast<float>(SheetSize.x);
		if (FlipHorizontal)
		{
			std::swap(U0, U1);
		}

		float V0 = (TilePos.y * TileSize.y) / static_cast<float>(SheetSize.y);
		float V1 = ((TilePos.y + 1) * TileSize.y) / static_cast<float>(SheetSize.y);
		if (FlipVertical)
		{
			const float InvV0 = 1.0f - V1;
			const float InvV1 = 1.0f - V0;
			V0 = InvV0;
			V1 = InvV1;
		}

		return FSpriteUV(U0, V0, U1, V1);
	}

	void CSprite::CalculateUV(FSpriteUV& SpriteUV, const glm::vec2& TilePos, const glm::vec2& TileSize, 
							  const glm::vec2& SheetSize, const bool FlipHorizontal, const bool FlipVertical)
	{
		SpriteUV.U0 = (TilePos.x * TileSize.x)  / static_cast<float>(SheetSize.x);
		SpriteUV.U1 = ((TilePos.x + 1) * TileSize.x) / static_cast<float>(SheetSize.x);
		if (FlipHorizontal)
		{
			std::swap(SpriteUV.U0, SpriteUV.U1);
		}

		SpriteUV.V0 = (TilePos.y * TileSize.y) / static_cast<float>(SheetSize.y);
		SpriteUV.V1 = ((TilePos.y + 1) * TileSize.y) / static_cast<float>(SheetSize.y);
		if (FlipVertical)
		{
			const float InvV0 = 1.0f - SpriteUV.V1;
			const float InvV1 = 1.0f - SpriteUV.V0;
			SpriteUV.V0 = InvV0;
			SpriteUV.V1 = InvV1;
		}
	}

	void CSprite::UpdateSprite(const bool FlipHorizontal, const bool FlipVertical)
	{
		UV = CalculateUV(TilePos, TileSize, Size, FlipHorizontal, FlipVertical);
	}

}