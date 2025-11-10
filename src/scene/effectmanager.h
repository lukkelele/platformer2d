#pragma once

#include <chrono>
#include <variant>
#include <vector>

#include <glm/glm.hpp>

#include "core/core.h"
#include "renderer/sprite.h"
#include "renderer/texture.h"

using namespace std::chrono_literals;

namespace platformer2d {

	enum class EEffect : uint16_t
	{
		Swoosh,
	};

	class CEffectManager
	{
	public:
		using TEffectTexture = std::variant<CSprite, CTexture>;
	public:
		CEffectManager();
		~CEffectManager() = default;

		static CEffectManager& Get();

		void Initialize();
		void Destroy();

		void Tick(float DeltaTime);
		void Play(EEffect Effect, const glm::vec2& Pos, std::chrono::milliseconds TimeActive,
				  const glm::vec2& Size = {0.15f, 0.15f}, float ZIndex = 1.0f);

		void RegisterEffect(EEffect Effect, std::shared_ptr<CSprite> EffectTexture);
		void RegisterEffect(EEffect Effect, std::shared_ptr<CTexture> EffectTexture);

	private:
		CEffectManager(const CEffectManager&) = delete;
		CEffectManager(CEffectManager&&) = delete;

	private:
		bool bInitialized = false;

		struct FEffectEntry
		{
			EEffect Effect;
			glm::vec2 Pos;
			std::chrono::steady_clock::time_point TimeExpire;
			glm::vec2 Size;
			float ZIndex = 1.0f;
		};
		std::vector<FEffectEntry> ActiveEffects;
		std::vector<decltype(ActiveEffects)::size_type> ExpiredIdx;

		std::unordered_map<EEffect, std::shared_ptr<TEffectTexture>> TextureMap;
	};

	namespace Enum
	{
		inline const char* ToString(const EEffect Effect)
		{
			const char* S = "";
		#define _(EnumValue) case EEffect::EnumValue: S = #EnumValue; break
			switch (Effect)
			{
				_(Swoosh);
				default:
					LK_THROW_ENUM_ERR(Effect);
					break;
			}
		#undef _
			return S;
		}
	}

}
