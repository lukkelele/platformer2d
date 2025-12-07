#include "rifle.h"

#include "renderer/renderer.h"
#include "physics/physicsworld.h"

namespace platformer2d {

	CRifle::CRifle()
	{
	}

	CRifle::~CRifle()
	{
		LK_DEBUG_TAG("Rifle", "Release: {}", Enum::ToString(GetType()));
	}

	void CRifle::Tick()
	{
		if (Owner) {
			Origin = Owner->GetPosition();
		}

		const auto TimeNow = std::chrono::steady_clock::now();
		for (const auto& Projectile : Fired) {
			LK_ASSERT(Projectile && b2Body_IsValid(Projectile->ID));
			const b2Vec2 Pos = b2Body_GetPosition(Projectile->ID);
			const float Angle = b2Rot_GetAngle(b2Body_GetRotation(Projectile->ID));
			const glm::vec3 P0 = { Pos.x, Pos.y, -0.010f };
			CRenderer::DrawCircleFilled(P0, ProjectileRadius, Projectile->GetColor(), 1.0f);

			if (TimeNow > (Projectile->TimeFired + ExpireTimeout)) {
				ExpiredQueue.push(Projectile->ID);
			}
		}

		/* Remove expired projectiles. */
		while (!ExpiredQueue.empty()) {
			b2BodyId& Expired = ExpiredQueue.front();
			DestroyProjectile(Expired);
			ExpiredQueue.pop();
		}
	}

	void CRifle::Fire(const glm::vec2& TargetPos)
	{
		if (!Owner || (Ammo <= 0)) {
			return;
		}

		b2BodyDef BodyDef = b2DefaultBodyDef();
		BodyDef.type = b2_dynamicBody;
		BodyDef.position = b2Vec2(Origin.x, Origin.y);
		BodyDef.isBullet = true;

		const glm::vec2 Diff = TargetPos - Origin;
		float LenSq = (Diff.x * Diff.x) + (Diff.y * Diff.y);
		if (LenSq <= 0.0000010f) {
			return;
		}

		const float InvLen = (1.0f / std::sqrt(LenSq));
		const b2Vec2 Dir = b2Vec2(Diff.x * InvLen, Diff.y * InvLen);
		BodyDef.linearVelocity = b2Vec2(ProjectileVelocity * Dir.x, ProjectileVelocity * Dir.y);

		/* Offset muzzle based on look direction. */
		if (BodyDef.linearVelocity.x < 0.0f) {
			BodyDef.position.x -= MuzzleOffset.x;
		} else if (BodyDef.linearVelocity.x > 0.0f) {
			BodyDef.position.x += MuzzleOffset.x;
		}

		FActorSpecification Spec;
		Spec.Name = LK_FMT("Projectile-{}", Ammo);
		Spec.Pos = glm::vec3(Origin, -0.10f);
		Spec.Color = ProjectileColor;
		std::shared_ptr<CProjectile> Projectile = std::make_shared<CProjectile>(Spec, this, &CRifle::DestroyProjectile);
		Projectile->ID = CPhysicsWorld::CreateBody(BodyDef);
		Projectile->bExplodeOnImpact = bProjectileExplodeOnImpact;
		Projectile->MaxBounceCount = ProjectileBounceCount;

		b2Circle Circle = { { 0.0f, 0.0f }, ProjectileRadius };
		b2ShapeDef ShapeDef = b2DefaultShapeDef();
		ShapeDef.userData = Projectile.get();
		ShapeDef.enableContactEvents = true;
		ShapeDef.material.restitution = ProjectileRestitution;
		Projectile->ShapeID = b2CreateCircleShape(Projectile->ID, &ShapeDef, &Circle);
		Projectile->TimeFired = std::chrono::steady_clock::now();
		Fired.push_back(Projectile);

		Ammo--;
		LK_TRACE_TAG("Rifle", "Fire: {} Ammo={} Velocity={}", Projectile->GetName(), Ammo, BodyDef.linearVelocity);
	}

	bool CRifle::Reload()
	{
		LK_TRACE_TAG("Rifle", "Begin reload with {} left in ammo", Ammo);
		
		/** @todo */
		Ammo = MAGAZINE_SIZE;

		return true;
	}

	void CRifle::Equip(CActor* Actor)
	{
		Owner = Actor;
	}

	bool CRifle::IsHeldBy(const CActor* Actor) const
	{
		return (Actor && (Owner == Actor));
	}

	void CRifle::SetEnabled(const bool Enabled)
	{
		bEnabled = Enabled;
	}

	void CRifle::SetProjectileRadius(const float InRadius)
	{
		LK_ASSERT(InRadius > 0.0f);
		ProjectileRadius = InRadius;
	}

	void CRifle::SetProjectileVelocity(const float InVelocity)
	{
		ProjectileVelocity = InVelocity;
	}

	void CRifle::SetProjectileRestitution(const float InRestitution)
	{
		ProjectileRestitution = InRestitution;
	}

	void CRifle::SetProjectileExplodeOnImpact(const bool ExplodeOnImpact)
	{
		bProjectileExplodeOnImpact = ExplodeOnImpact;
	}

	void CRifle::SetProjectileColor(const glm::vec4& InColor)
	{
		ProjectileColor = InColor;
	}

	bool CRifle::DestroyProjectile(const b2BodyId& ID)
	{
		const std::size_t Removed = std::erase_if(Fired, [&ID](std::shared_ptr<CProjectile> Projectile)
		{
			if (!B2_ID_EQUALS(ID, Projectile->ID)) {
				return false;
			}

			b2DestroyBody(ID);
			return true;
		});
		LK_ASSERT(Removed == 1, "Failed to remove projectile (Removed={})", Removed);
		return (Removed == 1);
	}

}
