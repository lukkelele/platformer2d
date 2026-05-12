#pragma once

#include "core/core.h"
#include "core/delegate.h"
#include "gamesystem.h"
#include "physics/events.h"

namespace platformer2d {

	class CActor;

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

	private:
		void OnContactBegin(const CContactBeginEvent& Event);
		void HandleProjectileHit(CActor* ProjectileActor, CActor* HitActor);

	private:
		Core::FDelegateHandle OnContactBeginHandle;
	};

}
