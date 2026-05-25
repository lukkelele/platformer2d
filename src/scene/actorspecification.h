#pragma once

#include "core/core.h"
#include "renderer/texture.h"
#include "renderer/color.h"

namespace platformer2d {

	enum class EActorType : std::uint16_t
	{
		Object,
		Player,
		Enemy,
		Spawnpoint,
		Projectile,
		COUNT
	};
	LK_ENUM(EActorType);

	enum EActorFlag : std::uint64_t
	{
		EActorFlag_None = 0,
		EActorFlag_Transparent = LK_BIT(1),
		EActorFlag_Terrain = LK_BIT(2),
		EActorFlag_Spawnpoint = LK_BIT(3),
	};

	struct FActorSpecification
	{
		LUUID Handle{};
		EActorType Type = EActorType::Object;
		ETexture Texture = ETexture::White;
		glm::vec4 Color = FColor::White;
		glm::vec3 Pos = {0.0f, 0.0f, 0.0f};
		std::string Name;
		EActorFlag Flags = EActorFlag::EActorFlag_None;

		bool OutlineEnabled = true;
		float OutlineThickness = 0.0f;
		glm::vec4 OutlineColor = FColor::Transparent;

		FActorSpecification() = default;
		FActorSpecification(const ETexture InTexture, std::string InName = "")
			: Texture(InTexture)
			, Name(std::move(InName))
		{}
	};

}

