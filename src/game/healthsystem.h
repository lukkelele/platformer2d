#pragma once

#include "core/core.h"
#include "gamesystem.h"

namespace platformer2d {

	class CActor;

	class CHealthSystem : public IGameSystem
	{
	public:
		CHealthSystem() = default;
		CHealthSystem(CHealthSystem&&) = delete;
		CHealthSystem(const CHealthSystem&) = delete;
		~CHealthSystem() = default;

		CHealthSystem& operator=(CHealthSystem&&) = delete;
		CHealthSystem& operator=(const CHealthSystem&) = delete;

		void Initialize(CGameInstance& Owner) override {}
		void Shutdown() override {}

		bool ApplyDamage(CActor* Target, float Amount);
		bool Heal(CActor* Target, float Amount);
		bool Kill(CActor* Target);
	};

}
