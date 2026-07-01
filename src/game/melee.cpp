#include "melee.h"

#include "core/log.h"
#include "instance.h"
#include "healthsystem.h"
#include "player.h"
#include "renderer/renderer.h"
#include "physics/physicsworld.h"

namespace platformer2d {

	CMelee::CMelee(const FMeleeSpecification& InSpec, CActor* InOwner)
		: Spec(InSpec)
		, Owner(InOwner)
	{
	}

	void CMelee::Tick(const float DeltaTime)
	{
		if (!Owner) {
			return;
		}

		if (Owner->IsPlayer()) {
			LookDir = Owner->As<CPlayer>().GetLookDirection();
		}

		if ((State != EMeleeState::Idle) && (DeltaTime > 0.0f)) {
			TickSwing(DeltaTime);
		}

		Render();
	}

	void CMelee::Render()
	{
		if (!Owner) {
			return;
		}

		const glm::vec3 Pos = Owner->GetPosition();
		const float DirSign = (LookDir == EDirection::Left) ? -1.0f : 1.0f;

		const float Progress = GetSwingProgress();
		const float BaseAngle = (State == EMeleeState::Idle) ? -25.0f : (-60.0f + (Progress * 150.0f));
		const float Angle = DirSign * BaseAngle;

		const glm::vec2 Size{Spec.Reach, Spec.HitboxHeight};
		const glm::vec3 RenderPos{
			Pos.x + (DirSign * (Spec.Reach * 0.5f)),
			Pos.y,
			-0.090f,
		};

		CRenderer::DrawQuad(
			RenderPos,
			Size,
			*CRenderer::GetTexture(ETexture::Axe),
			CRenderer::GetTextureCoords(LookDir),
			FColor::White,
			Angle);
	}

	void CMelee::PrimaryAction(const glm::vec2& TargetWorldPos)
	{
		LK_UNUSED(TargetWorldPos);
		Swing();
	}

	void CMelee::Equip(CActor* Actor)
	{
		Owner = Actor;
	}

	bool CMelee::Swing()
	{
		if (!Owner || (State != EMeleeState::Idle)) {
			return false;
		}

		State = EMeleeState::Windup;
		StateEntered = std::chrono::steady_clock::now();
		AlreadyHit.clear();
		LK_TRACE_TAG("Melee", "Swing - Start ({})", Enum::ToString(LookDir));
		return true;
	}

	void CMelee::TickSwing(const float DeltaTime)
	{
		using namespace std::chrono;
		const auto Now = steady_clock::now();
		const auto Elapsed = duration_cast<milliseconds>(Now - StateEntered);

		switch (State) {
			case EMeleeState::Idle:
				break;

			case EMeleeState::Windup:
				if (Elapsed >= Spec.WindupDuration) {
					State = EMeleeState::Active;
					StateEntered = Now;
				}
				break;

			case EMeleeState::Active:
				QueryAndDamage();
				if (Elapsed >= Spec.ActiveDuration) {
					State = EMeleeState::Recovery;
					StateEntered = Now;
				}
				break;

			case EMeleeState::Recovery:
				if (Elapsed >= Spec.RecoveryDuration) {
					State = EMeleeState::Idle;
					AlreadyHit.clear();
				}
				break;
		}
	}

	void CMelee::QueryAndDamage()
	{
		if (!Owner) {
			return;
		}

		const glm::vec3 Pos = Owner->GetPosition();
		const float DirSign = (LookDir == EDirection::Left) ? -1.0f : 1.0f;
		const float MinX = (DirSign > 0.0f) ? Pos.x : (Pos.x - Spec.Reach);
		const float MaxX = (DirSign > 0.0f) ? (Pos.x + Spec.Reach) : Pos.x;
		const b2AABB AABB = {
			.lowerBound = {MinX, Pos.y - (Spec.HitboxHeight * 0.50f)},
			.upperBound = {MaxX, Pos.y + (Spec.HitboxHeight * 0.50f)},
		};

		const b2QueryFilter Filter = b2DefaultQueryFilter();
		b2World_OverlapAABB(CPhysicsWorld::GetID(), AABB, Filter, &CMelee::OnOverlapStatic, this);
	}

	bool CMelee::OnOverlapStatic(const b2ShapeId ShapeId, void* Context)
	{
		CMelee* Ref = static_cast<CMelee*>(Context);
		return Ref->OnOverlap(ShapeId);
	}

	bool CMelee::OnOverlap(const b2ShapeId ShapeId)
	{
		CActor* HitActor = static_cast<CActor*>(b2Shape_GetUserData(ShapeId));
		if (!HitActor || (HitActor == Owner)) {
			return true;
		}

		auto Iter = std::find_if(AlreadyHit.cbegin(), AlreadyHit.cend(), ActorPred::IsEqual(HitActor));
		if (Iter != AlreadyHit.end()) {
			LK_TRACE_TAG("Melee", "Already hit: {}", (*Iter)->GetName());
			return true;
		}

		LK_DEBUG_TAG("Melee", "OnOverlap: {}", HitActor->GetName());
		auto& Health = CGameInstance::Get().GetSystem<CHealthSystem>();
		if (Health.IsDamageable(*HitActor)) {
			Health.ApplyDamage(*HitActor, Spec.Damage);
			AlreadyHit.push_back(HitActor);
			LK_TRACE_TAG("Melee", "Hit {} for {}", HitActor->GetName(), Spec.Damage);
		}

		return true;
	}

	float CMelee::GetSwingProgress() const
	{
		using namespace std::chrono;
		if (State == EMeleeState::Idle) {
			return 0.0f;
		}

		const auto Now = steady_clock::now();
		const float Elapsed = duration_cast<duration<float>>(Now - StateEntered).count();
		const float Total = GetDuration(State);

		switch (State) {
			case EMeleeState::Windup:
			{
				return (Total > 0.0f) ? (Elapsed / Total) * 0.30f : 0.30f;
			}

			case EMeleeState::Active:
			{
				const float P = (Total > 0.0f) ? (Elapsed / Total) : 1.0f;
				return 0.30f + (P * 0.60f);
			}

			case EMeleeState::Recovery:
			{
				const float P = (Total > 0.0f) ? (Elapsed / Total) : 1.0f;
				return 0.90f + (P * 0.10f);
			}

			case EMeleeState::Idle:
				LK_ASSERT(false);
				break;
		}

		return 0.0f;
	}

	float CMelee::GetDuration(const EMeleeState State) const
	{
		using namespace std::chrono;
		switch (State) {
			case EMeleeState::Windup:
				return duration_cast<duration<float>>(Spec.WindupDuration).count();
			case EMeleeState::Active:
				return duration_cast<duration<float>>(Spec.ActiveDuration).count();
			case EMeleeState::Recovery:
				return duration_cast<duration<float>>(Spec.RecoveryDuration).count();
			case EMeleeState::Idle:
				LK_ASSERT(false);
				break;
		}
		return 0.0f;
	}
}

