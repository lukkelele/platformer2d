#pragma once

#include <span>

#include "core/core.h"
#include "renderer/color.h"
#include "renderer/texture.h"
#include "physics/body.h"

namespace platformer2d {

	class CActor;

	class CSpawner
	{
	public:
		CSpawner() = default;
		~CSpawner() = default;
		CSpawner(const CSpawner&) = delete;
		CSpawner(CSpawner&&) = delete;

		static std::shared_ptr<CActor> CreateStaticPolygon(std::string_view Name, const glm::vec2& Pos,
			const glm::vec2& Size, const glm::vec4& Color = FColor::White, ETexture Texture = ETexture::White);
		static std::shared_ptr<CActor> CreatePolygon(std::string_view Name, const FBodySpecification& BodySpec,
			const glm::vec2& Size, const glm::vec4& Color = FColor::White, ETexture Texture = ETexture::White);
		static std::shared_ptr<CActor> CreateChain(std::string_view Name, std::span<const glm::vec2> Points,
			bool bLoop = false, const glm::vec4& Color = FColor::White);

		static std::shared_ptr<CActor> CreateSpawnpoint(std::string_view Name, const glm::vec2& Pos);
	};

}
