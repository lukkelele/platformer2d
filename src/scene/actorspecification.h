#pragma once

#include "core/core.h"
#include "renderer/texture.h"
#include "renderer/color.h"

namespace platformer2d {

	struct FActorSpecification
	{
		ETexture Texture = ETexture::White;
		glm::vec4 Color = FColor::White;
	};

}