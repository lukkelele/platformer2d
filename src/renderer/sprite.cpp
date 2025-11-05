#include "sprite.h"

namespace platformer2d {

	FSpriteUV GetSpriteUV(const glm::vec2& TilePos, const glm::vec2& TileSize,
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

	void GetSpriteUV(FSpriteUV& SpriteUV, const glm::vec2& TilePos, const glm::vec2& TileSize,
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