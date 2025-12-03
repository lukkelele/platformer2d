#pragma once

#include "core/core.h"
#include "renderer/texture.h"
#include "renderer/color.h"

namespace platformer2d {

	struct FActorSpecification
	{
		LUUID Handle{};
		ETexture Texture = ETexture::White;
		glm::vec4 Color = FColor::White;

		bool OutlineEnabled = true;
		float OutlineThickness = 0.0f;
		glm::vec4 OutlineColor = FColor::Transparent;

		FActorSpecification() = default;
		FActorSpecification(const ETexture InTexture) : Texture(InTexture) {}
	};

}