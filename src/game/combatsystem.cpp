#include "combatsystem.h"

#include "healthsystem.h"
#include "instance.h"
#include "physics/body.h"
#include "physics/physicsworld.h"
#include "scene/actor.h"
#include "scene/effectmanager.h"

namespace platformer2d {

	void CCombatSystem::Initialize(CGameInstance& Owner)
	{
		LK_DEBUG_TAG("CombatSystem", "Initialize");
		OwnerRef = &Owner;
		OnContactBeginHandle = CPhysicsWorld::OnContactBeginEvent.Add(this, &CCombatSystem::OnContactBegin);
	}

	void CCombatSystem::Shutdown()
	{
		LK_TRACE_TAG("CombatSystem", "Shutdown");
		CPhysicsWorld::OnContactBeginEvent.Remove(OnContactBeginHandle);
		OwnerRef = nullptr;
	}

	bool CCombatSystem::ApplyHit(CActor& Source, CActor& Target, const FHitSpec& Hit)
	{
		FHealthComponent* Health = Target.TryGetComponent<FHealthComponent>();
		if (!Health || !Health->IsHittable()) {
			return false;
		}

		LK_ASSERT(OwnerRef);
		if (Hit.Damage > 0.0f) {
			OwnerRef->GetSystem<CHealthSystem>().ApplyDamage(Target, Hit.Damage);
		}

		if (CBody* Body = Target.GetBody()) {
			const glm::vec2 SourcePos = glm::vec2(Source.GetPosition());
			const glm::vec2 TargetPos = glm::vec2(Target.GetPosition());
			const float DirX = ((TargetPos.x - SourcePos.x) >= 0.0f) ? 1.0f : -1.0f;
			const glm::vec2 Velocity = Body->GetLinearVelocity();
			Body->SetLinearVelocity({DirX * Hit.Knockback, std::max(Velocity.y, Hit.KnockbackUp)});
		}

		const glm::vec2 EffectPos = glm::vec2(Target.GetPosition());
		for (const EEffect Effect : Hit.Effects) {
			CEffectManager::Get().Play(Effect, EffectPos, 300ms);
		}
		Health->HitCooldownTimer = Hit.HitCooldownSeconds;

		LK_DEBUG_TAG("CombatSystem", "[{}] hit [{}] for {} damage", Source.GetName(), Target.GetName(), Hit.Damage);
		return true;
	}

	void CCombatSystem::OnContactBegin(const CContactBeginEvent& Event)
	{
		if (!Event.A || !Event.B) {
			return;
		}

		TryContactHit(*Event.A, *Event.B);
		TryContactHit(*Event.B, *Event.A);
	}

	void CCombatSystem::TryContactHit(CActor& Source, CActor& Target)
	{
		const FCombatComponent* Combat = Source.TryGetComponent<FCombatComponent>();
		if (!Combat || !Combat->bEnabled) {
			return;
		}

		if (Source.GetActorType() == Target.GetActorType()) {
			return;
		}

		ApplyHit(Source, Target, Combat->ContactHit);
	}

}
