#include "healthsystem.h"

#include <algorithm>

#include "scene/actor.h"

namespace platformer2d {

	bool CHealthSystem::ApplyDamage(CActor* const Target, const float Amount)
	{
		if (!Target || (Amount <= 0.0f)) {
			return false;
		}

		FHealthComponent* HC = Target->TryGetComponent<FHealthComponent>();
		if (!HC) {
			return false;
		}

		if (!HC->IsDamageable()) {
			LK_DEBUG_TAG("Health", "[{}] Damage skipped (not damageable)", Target->GetName());
			return false;
		}

		HC->SetHealth(HC->GetHealth() - Amount);
		LK_DEBUG_TAG("Health", "[{}] -{} HP -> {}/{}", Target->GetName(), Amount, HC->GetHealth(), HC->GetMaxHealth());

		if (HC->IsDead()) {
			LK_INFO_TAG("Health", "[{}] Killed", Target->GetName());
			Target->OnDeath();
			return true;
		}

		return false;
	}

	bool CHealthSystem::Heal(CActor* const Target, const float Amount)
	{
		if (!Target || Amount <= 0.0f) {
			return false;
		}

		FHealthComponent* HC = Target->TryGetComponent<FHealthComponent>();
		if (!HC) {
			return false;
		}

		const float NewHealth = std::min(HC->GetHealth() + Amount, HC->GetMaxHealth());
		HC->SetHealth(NewHealth);
		LK_DEBUG_TAG("Health", "[{}] +{} HP -> {}/{}", Target->GetName(), Amount, HC->GetHealth(), HC->GetMaxHealth());

		return true;
	}

	bool CHealthSystem::Kill(CActor* const Target)
	{
		if (!Target) {
			return false;
		}

		FHealthComponent* HC = Target->TryGetComponent<FHealthComponent>();
		if (!HC) {
			return false;
		}

		HC->SetHealth(0.0f);
		LK_INFO_TAG("Health", "[{}] Killed (forced)", Target->GetName());
		Target->OnDeath();

		return true;
	}
}

