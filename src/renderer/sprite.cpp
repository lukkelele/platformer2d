#include "sprite.h"

#include "texture.h"

namespace platformer2d {

	CSprite::CSprite(std::shared_ptr<CTexture> InTexture, const glm::vec2& InTilePos,
					 const glm::vec2& InTileSize, const bool InVerticalFlip)
		: Texture(InTexture)
		, Size(InTexture->GetWidth(), Texture->GetHeight())
		, TilePos(InTilePos)
		, TileSize(InTileSize)
		, bVerticalFlip(InVerticalFlip)
	{
		LK_VERIFY(Texture && (Size.x > 0.0f) && (Size.y > 0.0f)
				  && (TilePos.x > 0.0f) && (TilePos.y > 0.0f)
				  && (TileSize.x > 0.0f) && (TileSize.y > 0.0f));
		LK_DEBUG_TAG("Sprite", "Created: {} Size=({}, {}) TilePos=({}, {}) TileSize=({}, {})", InTexture->GetFilePath().filename(),
					 Size.x, Size.y, TilePos.x, TilePos.y, TileSize.x, TileSize.y);

		UV = CalculateUV(TilePos, TileSize, Size, bVerticalFlip);
	}

	CSprite::~CSprite()
	{
		if (Texture)
		{
			Texture.reset();
		}
	}

	void CSprite::SetTilePos(const uint16_t X, const uint16_t Y)
	{
		SetTilePos({ X, Y });
	}

	void CSprite::SetTilePos(const glm::vec2& InTilePos)
	{
		TilePos = InTilePos;
	}

	std::size_t CSprite::IncrementTilePosX(const std::size_t Times)
	{
		LK_ASSERT(Times > 0);
		TilePos.x += Times;
		return TilePos.x;
	}

	std::size_t CSprite::DecrementTilePosX(const std::size_t Times)
	{
		LK_ASSERT(Times > 0);
		TilePos.x -= Times;
		return TilePos.x;
	}

	FSpriteUV CSprite::CalculateUV(const glm::vec2& TilePos, const glm::vec2& TileSize,
								   const glm::vec2& SheetSize, const bool VerticalFlip)
	{
		const float U0 = (TilePos.x * TileSize.x)  / static_cast<float>(SheetSize.x);
		const float U1 = ((TilePos.x + 1) * TileSize.x) / static_cast<float>(SheetSize.x);

		float V0 = (TilePos.y * TileSize.y) / static_cast<float>(SheetSize.y);
		float V1 = ((TilePos.y + 1) * TileSize.y) / static_cast<float>(SheetSize.y);
		if (VerticalFlip)
		{
			const float InvV0 = 1.0f - V1;
			const float InvV1 = 1.0f - V0;
			V0 = InvV0;
			V1 = InvV1;
		}

		return FSpriteUV(U0, V0, U1, V1);
	}

	void CSprite::CalculateUV(FSpriteUV& SpriteUV, const glm::vec2& TilePos, const glm::vec2& TileSize, 
							  const glm::vec2& SheetSize, const bool VerticalFlip)
	{
		SpriteUV.U0 = (TilePos.x * TileSize.x)  / static_cast<float>(SheetSize.x);
		SpriteUV.U1 = ((TilePos.x + 1) * TileSize.x) / static_cast<float>(SheetSize.x);

		SpriteUV.V0 = (TilePos.y * TileSize.y) / static_cast<float>(SheetSize.y);
		SpriteUV.V1 = ((TilePos.y + 1) * TileSize.y) / static_cast<float>(SheetSize.y);
		if (VerticalFlip)
		{
			const float InvV0 = 1.0f - SpriteUV.V1;
			const float InvV1 = 1.0f - SpriteUV.V0;
			SpriteUV.V0 = InvV0;
			SpriteUV.V1 = InvV1;
		}
	}

}