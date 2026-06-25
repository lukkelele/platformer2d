#pragma once

#include <chrono>
#include <variant>
#include <vector>

#include <glm/glm.hpp>

#include "core/core.h"
#include "renderer/sprite.h"
#include "renderer/texture.h"
#include "scene/components.h"

using namespace std::chrono_literals;

namespace platformer2d {

	/* @todo: Implement as IGameSystem */
	class CEffectManager
	{
	public:
		using TEffectTexture = std::variant<CSprite, CTexture>;

	public:
		CEffectManager();
		CEffectManager(CEffectManager&&) = delete;
		CEffectManager(const CEffectManager&) = delete;
		~CEffectManager() = default;

		CEffectManager& operator=(CEffectManager&&) = delete;
		CEffectManager& operator=(const CEffectManager&) = delete;

		static CEffectManager& Get();

		void Initialize();
		void Destroy();

		void Tick(float DeltaTime);
		void Play(EEffect Effect, const glm::vec2& Pos, std::chrono::milliseconds TimeActive,
			const glm::vec2& Size = {0.15f, 0.15f}, float ZIndex = 1.0f,
			const glm::vec2& Velocity = {0.0f, 0.0f});

		void RegisterEffect(EEffect Effect, std::shared_ptr<CSprite> EffectTexture);
		void RegisterEffect(EEffect Effect, std::shared_ptr<CTexture> EffectTexture);

	private:
		bool bInitialized = false;

		struct FEffectEntry
		{
			EEffect Effect;
			glm::vec2 Pos;
			std::chrono::steady_clock::time_point TimeExpire;
			glm::vec2 Size;
			float ZIndex = 1.0f;
			glm::vec2 Velocity = {0.0f, 0.0f};
		};
		std::vector<FEffectEntry> ActiveEffects;
		std::vector<decltype(ActiveEffects)::size_type> ExpiredIdx;

		std::unordered_map<EEffect, std::shared_ptr<TEffectTexture>> TextureMap;
	};
}

