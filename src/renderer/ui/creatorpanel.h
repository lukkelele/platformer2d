#pragma once

#include <glm/glm.hpp>

#include "renderer/color.h"
#include "renderer/texture.h"

namespace platformer2d {
	class CScene;
}

namespace platformer2d::UI {

	struct FActorAttributes
	{
		glm::vec2 Position = {0.0f, 0.0f};
		glm::vec2 Size = {0.20f, 0.20f};
		ETexture Texture = ETexture::White;
		EColor Color = EColor::White;
		std::array<char, 128> NameBuf = {0};
		bool bPreview = true;
		bool bPreviewSelected = false;
	};
	extern FActorAttributes ActorAttr;

	bool ActorAttributes(FActorAttributes& Attr);

	void Creator(std::shared_ptr<CScene> Scene);
	void ActorCreateButtons(std::shared_ptr<CScene> Scene);
	void RenderActorPreview(const std::shared_ptr<CScene>& Scene);
}

