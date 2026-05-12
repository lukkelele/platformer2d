#include "projectilesystem.h"

#include "healthsystem.h"
#include "instance.h"
#include "player.h"
#include "projectile.h"
#include "physics/physicsworld.h"
#include "scene/actor.h"

namespace platformer2d {

	void CProjectileSystem::Initialize(CGameInstance& Owner)
	{
		LK_DEBUG_TAG("ProjectileSystem", "Initialize");
		OwnerRef = &Owner;
		OnContactBeginHandle = CPhysicsWorld::OnContactBeginEvent.Add(this, &CProjectileSystem::OnContactBegin);
	}

	void CProjectileSystem::Shutdown()
	{
		LK_DEBUG_TAG("ProjectileSystem", "Shutdown");
		CPhysicsWorld::OnContactBeginEvent.Remove(OnContactBeginHandle);
		OwnerRef = nullptr;
	}

	void CProjectileSystem::OnContactBegin(const CContactBeginEvent& Event)
	{
		LK_TRACE_TAG("Projectile", "OnContactBegin: A={} B={}", (Event.A ? Event.A->GetName() : "NULL"), (Event.B ? Event.B->GetName() : "NULL"));
		LK_ASSERT(Event.A && Event.B, "Invalid event references");
		if (!Event.A || !Event.B) {
			return;
		}

		const EActorType AType = Event.A->GetActorType();
		const EActorType BType = Event.B->GetActorType();
		if (AType == EActorType::Projectile) {
			HandleProjectileHit(Event.A, Event.B);
		} else if (BType == EActorType::Projectile) {
			HandleProjectileHit(Event.B, Event.A);
		}
	}

	void CProjectileSystem::HandleProjectileHit(CActor* ProjectileActor, CActor* HitActor)
	{
		CProjectile* Projectile = static_cast<CProjectile*>(ProjectileActor);
		if (Projectile->GetOwner() && Projectile->GetOwner()->IsHeldBy(HitActor)) {
			return;
		}

		Projectile->BounceCount++;
		LK_ASSERT(HitActor, "Invalid projectile hit");
		LK_TRACE("{}: Hit: {} ({})", ProjectileActor->GetName(), HitActor->GetName(), Enum::ToString(HitActor->GetActorType()));

		LK_ASSERT(OwnerRef);
		OwnerRef->GetSystem<CHealthSystem>().ApplyDamage(HitActor, Projectile->GetDamage());

		if (Projectile->ExplodesOnImpact()) {
			Projectile->Destroy();
		} else if (Projectile->BounceCount >= Projectile->MaxBounceCount) {
			LK_TRACE("{}: Max bounce reached: {}", Projectile->GetName(), Projectile->BounceCount);
			Projectile->Destroy();
		}
	}

}
