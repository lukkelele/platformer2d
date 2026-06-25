#pragma once

#include <queue>
#include <string_view>
#include <vector>

#include "core/core.h"
#include "core/delegate.h"
#include "gamesystem.h"
#include "projectile.h"
#include "physics/events.h"

namespace platformer2d {

	class CActor;

	struct FProjectileSpawnParams
	{
		CActor* Spawner = nullptr;
		glm::vec2 Position{0.0f, 0.0f};
		glm::vec2 Velocity{0.0f, 0.0f};
		float Radius = 0.040f;
		float Restitution = 0.42f;
		float Damage = 10.0f;
		glm::vec4 Color = FColor::Magenta;
		std::uint8_t MaxBounceCount = 1;
		bool bExplodeOnImpact = false;
		bool bIsBullet = false;
		float GravityScale = 1.0f;
		float RenderZ = -0.020f;
		std::chrono::milliseconds ExpireTimeout = 3000ms;
		std::string_view NamePrefix = "Projectile";
	};

	class CProjectileSystem : public IGameSystem
	{
	public:
		CProjectileSystem() = default;
		CProjectileSystem(CProjectileSystem&&) = delete;
		CProjectileSystem(const CProjectileSystem&) = delete;
		~CProjectileSystem() = default;

		CProjectileSystem& operator=(CProjectileSystem&&) = delete;
		CProjectileSystem& operator=(const CProjectileSystem&) = delete;

		void Initialize(CGameInstance& Owner) override;
		void Shutdown() override;
		void Tick() override;

		CProjectile* Spawn(const FProjectileSpawnParams& Params);

	private:
		void OnContactBegin(const CContactBeginEvent& Event);
		void HandleProjectileHit(CActor* ProjectileActor, CActor* HitActor);
		void RenderProjectile(const CProjectile& Projectile) const;
		bool Destroy(const b2BodyId& ID);
		void DestroyExpired();

	private:
		Core::FDelegateHandle OnContactBeginHandle;
		std::vector<std::shared_ptr<CProjectile>> Live;
		std::queue<b2BodyId> ExpiredQueue;
	};

}
