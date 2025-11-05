#pragma once

#include <glm/glm.hpp>

#include "core/core.h"

namespace platformer2d {

	struct FSpriteUV
	{
		float U0 = 0.0f;
		float V0 = 0.0f;
		float U1 = 0.0f;
		float V1 = 0.0f;
	};

	FSpriteUV GetSpriteUV(const glm::vec2& TilePos, const glm::vec2& TileSize, 
						  const glm::vec2& SheetSize, bool VerticalFlip = false);
	void GetSpriteUV(FSpriteUV& SpriteUV, const glm::vec2& TilePos, const glm::vec2& TileSize,
					 const glm::vec2& SheetSize, bool VerticalFlip = false);

}
