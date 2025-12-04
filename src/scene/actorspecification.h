#pragma once

#include "core/core.h"
#include "renderer/texture.h"
#include "renderer/color.h"

namespace platformer2d {

	enum class EActorType : uint16_t
	{
		Object,
		Player,
		Spawnpoint,
	};

	struct FActorSpecification
	{
		LUUID Handle{};
		EActorType Type = EActorType::Object;
		ETexture Texture = ETexture::White;
		glm::vec4 Color = FColor::White;
		glm::vec3 Pos = { 0.0f, 0.0f, 0.0f };
		std::string Name;

		bool OutlineEnabled = true;
		float OutlineThickness = 0.0f;
		glm::vec4 OutlineColor = FColor::Transparent;

		FActorSpecification() = default;
		FActorSpecification(const ETexture InTexture) : Texture(InTexture) {}
	};

}