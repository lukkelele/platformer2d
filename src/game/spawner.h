#pragma once

#include <span>
#include <vector>

#include "core/core.h"
#include "renderer/color.h"
#include "renderer/texture.h"
#include "physics/body.h"
#include "game/enemyspawner.h"

namespace platformer2d {

	class CActor;
	class CEnemy;

	class CSpawner
	{
	public:
		CSpawner() = default;
		~CSpawner() = default;
		CSpawner(const CSpawner&) = delete;
		CSpawner(CSpawner&&) = delete;

		CSpawner& operator=(const CSpawner&) = delete;
		CSpawner& operator=(CSpawner&&) = delete;

		static std::shared_ptr<CActor> CreateStaticPolygon(std::string_view Name, const glm::vec2& Pos,
			const glm::vec2& Size, const glm::vec4& Color = FColor::White, ETexture Texture = ETexture::White);
		static std::shared_ptr<CActor> CreateStaticPolygon(std::string_view Name, const glm::vec2& Pos,
			const glm::vec2& Size, const FBodySpecification& BodySpec, const glm::vec4& Color = FColor::White, ETexture Texture = ETexture::White);
		static std::shared_ptr<CActor> CreatePolygon(std::string_view Name, const FBodySpecification& BodySpec,
			const glm::vec2& Size, const glm::vec4& Color = FColor::White, ETexture Texture = ETexture::White);
		static std::shared_ptr<CActor> CreateChain(std::string_view Name, std::span<const glm::vec2> Points,
			bool Loop = false, bool BlockBothSides = false, const glm::vec4& Color = FColor::White);

		static std::shared_ptr<CActor> CreateSpawnpoint(std::string_view Name, const glm::vec2& Pos);

		static std::shared_ptr<CEnemy> CreateEnemy(EEnemyArchetype Archetype, const glm::vec2& Pos,
			std::string_view Name = "", ETexture Texture = ETexture::Goblin);
		static std::shared_ptr<CEnemySpawner> CreateEnemySpawner(std::string_view Name, const glm::vec2& Pos,
			std::vector<FSpawnWave> Waves = {});
	};

}
