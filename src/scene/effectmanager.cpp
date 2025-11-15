#include "effectmanager.h"

#include "renderer/renderer.h"

namespace platformer2d {

	CEffectManager::CEffectManager()
	{
		ExpiredIdx.reserve(10);
	}

	CEffectManager& CEffectManager::Get()
	{
		static CEffectManager Instance;
		return Instance;
	}

	void CEffectManager::Initialize()
	{
		LK_VERIFY(bInitialized == false, "Initialize called multiple times");
		LK_TRACE_TAG("EffectManager", "Initialize");

		{
			FSpriteAnimation SwooshAnim;
			constexpr glm::vec2 TilePos = { 0, 0 };
			SwooshAnim.StartTileX = TilePos.x;
			SwooshAnim.StartTileY = TilePos.y;
			SwooshAnim.FrameCount = 4;
			SwooshAnim.TicksPerFrame = 18;
			constexpr glm::vec2 TileSize = { 32, 32 };
			std::shared_ptr<CSprite> SwooshSprite = std::make_shared<CSprite>(
				CRenderer::GetTexture(ETexture::Swoosh),
				TilePos, 
				TileSize, 
				SwooshAnim
			);

			RegisterEffect(EEffect::Swoosh, SwooshSprite);
		}

		bInitialized = true;
	}

	void CEffectManager::Destroy()
	{
		LK_DEBUG_TAG("EffectManager", "Destroy");
	}

	void CEffectManager::Tick(const float DeltaTime)
	{
		using namespace std::chrono;
		const auto CurrentTime = high_resolution_clock::now();

		int Idx = 0;
		ExpiredIdx.clear();
		for (FEffectEntry& Entry : ActiveEffects)
		{
			/* Render the effect. */
			if (CurrentTime <= Entry.TimeExpire)
			{
				std::shared_ptr<TEffectTexture>& EffectTex = TextureMap.at(Entry.Effect);

				std::visit([&Entry](auto& EffectTex)
				{
					using T = std::decay_t<decltype(EffectTex)>;
					if constexpr (std::is_same_v<T, CSprite>)
					{
						CSprite& S = EffectTex;
						const FSpriteAnimation& Anim = S.GetAnimation();
						const uint16_t FrameIndex = CRenderer::GetFrameIndex();
						const uint16_t AnimFrame = Anim.CalculateAnimFrame(FrameIndex);
						S.SetTilePos(AnimFrame, Anim.StartTileY);

						CRenderer::DrawQuad(
							glm::vec3(Entry.Pos, Entry.ZIndex),
							Entry.Size,
							*S.GetTexture(),
							S.GetUV(),
							FColor::White
						);
					}
					else if constexpr (std::is_same_v<T, CTexture>)
					{
						const CTexture& Tex = EffectTex;
						CRenderer::DrawQuad(
							glm::vec3(Entry.Pos, Entry.ZIndex),
							Entry.Size,
							Tex,
							FColor::White
						);
					}

				}, *EffectTex);
			}
			else
			{
				LK_TRACE_TAG("EffectManager", "Expired: {}", Enum::ToString(Entry.Effect));
				ExpiredIdx.push_back(Idx);
			}

			Idx++;
		}

		if (!ExpiredIdx.empty())
		{
			/* Sort in reverse. */
			std::sort(ExpiredIdx.rbegin(), ExpiredIdx.rend());

			for (const int Idx : ExpiredIdx)
			{
				LK_TRACE_TAG("EffectManager", "Remove effect: {}", Idx);
				ActiveEffects.erase(ActiveEffects.begin() + Idx);
			}

			ExpiredIdx.clear();
		}
	}

	void CEffectManager::Play(EEffect Effect, const glm::vec2& Pos, std::chrono::milliseconds TimeActive,
							  const glm::vec2& Size, const float ZIndex)
	{
		using namespace std::chrono;
		LK_TRACE_TAG("EffectManager", "Play: {} -> {} ({}ms)", Enum::ToString(Effect), Pos, TimeActive.count());
		const FEffectEntry Entry = {
			.Effect = Effect,
			.Pos = Pos,
			.TimeExpire = high_resolution_clock::now() + TimeActive,
			.Size = Size,
			.ZIndex = ZIndex,
		};
		ActiveEffects.emplace_back(Entry);
	}

	void CEffectManager::RegisterEffect(const EEffect Effect, std::shared_ptr<CSprite> EffectTexture)
	{
		std::shared_ptr<TEffectTexture> Ptr = std::make_shared<TEffectTexture>(*EffectTexture);
		TextureMap.emplace(Effect, Ptr);
	}

	void CEffectManager::RegisterEffect(const EEffect Effect, std::shared_ptr<CTexture> EffectTexture)
	{
		std::shared_ptr<TEffectTexture> Ptr = std::make_shared<TEffectTexture>(*EffectTexture);
		TextureMap.emplace(Effect, Ptr);
	}

}
