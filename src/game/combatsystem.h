#pragma once

#include "core/core.h"
#include "core/delegate.h"
#include "gamesystem.h"
#include "physics/events.h"
#include "scene/components.h"

namespace platformer2d {

	class CActor;

	class CCombatSystem : public IGameSystem
	{
	public:
		CCombatSystem() = default;
		CCombatSystem(CCombatSystem&&) = delete;
		CCombatSystem(const CCombatSystem&) = delete;
		~CCombatSystem() = default;

		CCombatSystem& operator=(CCombatSystem&&) = delete;
		CCombatSystem& operator=(const CCombatSystem&) = delete;

		void Initialize(CGameInstance& Owner) override;
		void Shutdown() override;

		bool ApplyHit(CActor& Source, CActor& Target, const FHitSpec& Hit);

	private:
		void OnContactBegin(const CContactBeginEvent& Event);
		void TryContactHit(CActor& Source, CActor& Target);

	private:
		Core::FDelegateHandle OnContactBeginHandle;
	};

}
