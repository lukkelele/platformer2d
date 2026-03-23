#pragma once

#include "core/core.h"
#include "renderer/texture.h"
#include "renderer/color.h"

namespace platformer2d {

	enum class EActorType : uint16_t
	{
		Object,
		Player,
		Enemy,
		Spawnpoint,
		Projectile,
		COUNT
	};

	struct FActorSpecification
	{
		LUUID Handle{};
		EActorType Type = EActorType::Object;
		ETexture Texture = ETexture::White;
		glm::vec4 Color = FColor::White;
		glm::vec3 Pos = {0.0f, 0.0f, 0.0f};
		std::string Name;

		bool OutlineEnabled = true;
		float OutlineThickness = 0.0f;
		glm::vec4 OutlineColor = FColor::Transparent;

		FActorSpecification() = default;
		FActorSpecification(const ETexture InTexture)
			: Texture(InTexture)
		{}
	};

	namespace Enum {
		inline const char* ToString(const EActorType Type)
		{
			const char* S = "";
#define _(EnumValue)                                  \
	case EActorType::EnumValue: S = #EnumValue; break
			switch (Type) {
				_(Object);
				_(Player);
				_(Enemy);
				_(Spawnpoint);
				_(Projectile);
				_(COUNT);
				default:
					LK_THROW_ENUM_ERR(Type);
					break;
			}
#undef _
			return S;
		}
	}

}
