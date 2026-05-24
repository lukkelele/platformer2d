#include "projectilesystem.h"

#include "healthsystem.h"
#include "instance.h"
#include "player.h"
#include "projectile.h"
#include "physics/physicsworld.h"
#include "renderer/renderer.h"
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

		LK_TRACE_TAG("ProjectileSystem", "Destroying {} projectiles", Live.size());
		for (const std::shared_ptr<CProjectile>& Projectile : Live) {
			ExpiredQueue.push(Projectile->ID);
		}
		DestroyExpired();

		OwnerRef = nullptr;
	}

	CProjectile* CProjectileSystem::Spawn(const FProjectileSpawnParams& Params)
	{
		b2BodyDef BodyDef = b2DefaultBodyDef();
		BodyDef.type = b2_dynamicBody;
		BodyDef.position = {Params.Position.x, Params.Position.y};
		BodyDef.linearVelocity = {Params.Velocity.x, Params.Velocity.y};
		BodyDef.gravityScale = Params.GravityScale;
		BodyDef.isBullet = Params.bIsBullet;

		FActorSpecification Spec;
		Spec.Name = std::format("{}-{}", Params.NamePrefix, Live.size());
		Spec.Pos = {Params.Position.x, Params.Position.y, Params.RenderZ};
		Spec.Color = Params.Color;

		auto Projectile = std::make_shared<CProjectile>(Spec, Params.Spawner);
		Projectile->ID = CPhysicsWorld::CreateBody(BodyDef);
		Projectile->bExplodeOnImpact = Params.bExplodeOnImpact;
		Projectile->MaxBounceCount = Params.MaxBounceCount;
		Projectile->Damage = Params.Damage;
		Projectile->Radius = Params.Radius;
		Projectile->RenderZ = Params.RenderZ;
		Projectile->ExpireTimeout = Params.ExpireTimeout;

		b2Circle Circle = {{0.0f, 0.0f}, Params.Radius};
		b2ShapeDef ShapeDef = b2DefaultShapeDef();
		ShapeDef.userData = Projectile.get();
		ShapeDef.enableContactEvents = true;
		ShapeDef.material.restitution = Params.Restitution;
		Projectile->ShapeID = b2CreateCircleShape(Projectile->ID, &ShapeDef, &Circle);
		Projectile->TimeFired = std::chrono::steady_clock::now();

		CProjectile* Raw = Projectile.get();
		Live.push_back(std::move(Projectile));
		LK_TRACE_TAG("ProjectileSystem", "Spawn: {} (active={})", Raw->GetName(), Live.size());
		return Raw;
	}

	void CProjectileSystem::Tick()
	{
		if (Live.empty()) {
			return;
		}

		const float DeltaTime = OwnerRef ? OwnerRef->GetDeltaTime() : 0.0f;
		const auto TimeNow = std::chrono::steady_clock::now();
		if (DeltaTime > 0.0f) {
			for (const std::shared_ptr<CProjectile>& Projectile : Live) {
				LK_ASSERT(Projectile && b2Body_IsValid(Projectile->ID));
				RenderProjectile(*Projectile);
				if (TimeNow > (Projectile->TimeFired + Projectile->ExpireTimeout)) {
					ExpiredQueue.push(Projectile->ID);
				}
			}
		} else {
			using namespace std::chrono;
			const duration<float> DeltaSeconds{DeltaTime};
			for (const std::shared_ptr<CProjectile>& Projectile : Live) {
				RenderProjectile(*Projectile);
				Projectile->TimeFired += duration_cast<steady_clock::duration>(DeltaSeconds);
			}
		}

		DestroyExpired();
	}

	void CProjectileSystem::RenderProjectile(const CProjectile& Projectile) const
	{
		const b2Vec2 Pos = b2Body_GetPosition(Projectile.ID);
		const glm::vec3 P0 = {Pos.x, Pos.y, Projectile.RenderZ};
		CRenderer::DrawCircleFilled(P0, Projectile.Radius, Projectile.GetColor(), 1.0f);
	}

	bool CProjectileSystem::Destroy(const b2BodyId& ID)
	{
		const std::size_t Removed = std::erase_if(Live, [&ID](const std::shared_ptr<CProjectile>& Projectile)
		{
			if (!Projectile || !b2Body_IsValid(ID) || B2_IS_NULL(ID) || B2_IS_NULL(Projectile->ID) || !B2_ID_EQUALS(ID, Projectile->ID)) {
				return false;
			}

			b2DestroyBody(ID);
			return true;
		});
		return (Removed == 1);
	}

	void CProjectileSystem::DestroyExpired()
	{
		while (!ExpiredQueue.empty()) {
			b2BodyId& Expired = ExpiredQueue.front();
			Destroy(Expired);
			ExpiredQueue.pop();
		}
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
		if (Projectile->GetSpawner() == HitActor) {
			return;
		}

		Projectile->BounceCount++;
		LK_ASSERT(HitActor, "Invalid projectile hit");
		LK_TRACE("{}: Hit: {} ({})", ProjectileActor->GetName(), HitActor->GetName(), Enum::ToString(HitActor->GetActorType()));

		LK_ASSERT(OwnerRef);
		OwnerRef->GetSystem<CHealthSystem>().ApplyDamage(*HitActor, Projectile->GetDamage());

		if (Projectile->ExplodesOnImpact()) {
			ExpiredQueue.push(Projectile->ID);
		} else if (Projectile->BounceCount >= Projectile->MaxBounceCount) {
			LK_TRACE("{}: Max bounce reached: {}", Projectile->GetName(), Projectile->BounceCount);
			ExpiredQueue.push(Projectile->ID);
		}
	}

}
