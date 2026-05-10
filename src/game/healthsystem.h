#pragma once

#include "core/core.h"

namespace platformer2d {

	class CActor;

	class CHealthSystem
	{
	public:
		CHealthSystem() = delete;
		~CHealthSystem() = delete;
		CHealthSystem(CHealthSystem&&) = delete;
		CHealthSystem(const CHealthSystem&) = delete;

		CHealthSystem& operator=(CHealthSystem&&) = delete;
		CHealthSystem& operator=(const CHealthSystem&) = delete;

		static bool ApplyDamage(CActor* Target, float Amount);
		static bool Heal(CActor* Target, float Amount);
		static bool Kill(CActor* Target);
	};

}
