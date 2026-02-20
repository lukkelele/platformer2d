#include "rifle.h"

#include "game/player.h"
#include "renderer/renderer.h"
#include "physics/physicsworld.h"

namespace platformer2d {

	CRifle::CRifle(const FRifleSpecification& InSpec, CActor* InOwner)
		: Owner(InOwner)
		, MagazineSize(InSpec.MagazineSize)
		, Ammo(InSpec.MagazineSize)
	{
	}

	CRifle::~CRifle()
	{
		LK_DEBUG_TAG("Rifle", "Release: {}", Enum::ToString(GetWeaponType()));

		LK_TRACE_TAG("Rifle", "Destroying {} projectiles", Fired.size());
		for (const std::shared_ptr<CProjectile> Projectile : Fired) {
			ExpiredQueue.push(Projectile->ID);
		}

		DestroyExpiredProjectiles();

		LK_ASSERT(ExpiredQueue.empty(), "ExpiredQueue not empty: {}", ExpiredQueue.size());
	}

	void CRifle::Tick(const float DeltaTime)
	{
		if (Owner) {
			Origin = Owner->GetPosition();
		}

		Render();

		if (DeltaTime > 0.0f) {
			const auto TimeNow = std::chrono::steady_clock::now();
			for (const auto& Projectile : Fired) {
				LK_ASSERT(Projectile && b2Body_IsValid(Projectile->ID));
				RenderProjectile(Projectile);

				if (TimeNow > (Projectile->TimeFired + ExpireTimeout)) {
					ExpiredQueue.push(Projectile->ID);
				}
			}
		} else {
			/* Adjust expire time if paused. */
			using namespace std::chrono;
			/**
			 * @todo: The calculated time will never truly match the difference from steady clock.
			 * Need a better way to handle the timestep if paused.
			 */
			const std::chrono::duration<float> DeltaSeconds{ DeltaTime };
			for (auto& Projectile : Fired) {
				RenderProjectile(Projectile);
				Projectile->TimeFired += duration_cast<steady_clock::duration>(DeltaSeconds);
			}
		}

		DestroyExpiredProjectiles();
	}

	void CRifle::Render()
	{
		const std::array<glm::vec2, 4>* TexCoords = &CRenderer::TextureCoords;
		if (LookDir == EDirection::Left) {
			TexCoords = &CRenderer::MirroredTextureCoords;
		} else {
			TexCoords = &CRenderer::TextureCoords;
		}

		/* Render rifle. */
		const float OffsetX = ((LookDir == EDirection::Left) ? -MuzzleOffset.x : MuzzleOffset.x);
		CRenderer::DrawQuad(
			glm::vec3(Origin.x + OffsetX + 0.0020f, Origin.y - MuzzleOffset.y - 0.0090f, -0.10f),
			glm::vec2(0.15f, 0.10f),
			*CRenderer::GetTexture(ETexture::Rifle),
			std::span<const glm::vec2, 4>(*TexCoords)
		);
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

		const glm::vec2 Diff = TargetPos - glm::vec2(Origin.x, Origin.y);
		float LenSq = (Diff.x * Diff.x) + (Diff.y * Diff.y);
		if (LenSq <= 0.0000010f) {
			return;
		}

		const float InvLen = (1.0f / std::sqrt(LenSq));
		const b2Vec2 Dir = b2Vec2(Diff.x * InvLen, Diff.y * InvLen);
		BodyDef.linearVelocity = b2Vec2(ProjectileVelocity * Dir.x, ProjectileVelocity * Dir.y);

		/* Offset muzzle based on look direction. */
		if (BodyDef.linearVelocity.x < 0.0f) {
			/* Left */
			BodyDef.position.x -= MuzzleOffset.x;
			RequestLookDirection(EDirection::Left);
		} else if (BodyDef.linearVelocity.x > 0.0f) {
			/* Right */
			BodyDef.position.x += MuzzleOffset.x;
			RequestLookDirection(EDirection::Right);
		}

		FActorSpecification Spec;
		Spec.Name = Format("Projectile-{}", Ammo);
		Spec.Pos = Origin;
		Spec.Color = ProjectileColor;
		std::shared_ptr<CProjectile> Projectile = std::make_shared<CProjectile>(Spec, this, &CRifle::DestroyProjectile);
		Projectile->ID = CPhysicsWorld::CreateBody(BodyDef);
		Projectile->bExplodeOnImpact = bProjectileExplodeOnImpact;
		Projectile->MaxBounceCount = ProjectileBounceCount;
		Projectile->Damage = ProjectileDamage;

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
		Ammo = MagazineSize;
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

	void CRifle::SetLookDirection(const EDirection InDirection)
	{
		LookDir = InDirection;
	}

	void CRifle::RequestLookDirection(const EDirection InDirection)
	{
		LK_TRACE_TAG("Rifle", "Request direction: {}", Enum::ToString(InDirection));
		if (Owner && Owner->IsPlayer()) {
			Owner->As<CPlayer>().SetLookDirection(InDirection);
		} else {
			LK_ERROR_TAG("Rifle", "Failed to request direction: {}", Enum::ToString(InDirection));
		}
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

	void CRifle::RenderProjectile(const std::shared_ptr<CProjectile>& Projectile) const
	{
		const b2Vec2 Pos = b2Body_GetPosition(Projectile->ID);
		const float Angle = b2Rot_GetAngle(b2Body_GetRotation(Projectile->ID));
		const glm::vec3 P0 = { Pos.x, Pos.y, -0.010f };
		CRenderer::DrawCircleFilled(P0, ProjectileRadius, Projectile->GetColor(), 1.0f);
	}

	bool CRifle::DestroyProjectile(const b2BodyId& ID)
	{
		const std::size_t Removed = std::erase_if(Fired, [&ID](std::shared_ptr<CProjectile> Projectile)
		{
			if (!Projectile || !b2Body_IsValid(ID) || B2_IS_NULL(ID) || B2_IS_NULL(Projectile->ID) || !B2_ID_EQUALS(ID, Projectile->ID)) {
				return false;
			}

			b2DestroyBody(ID);
			return true;
		});
		LK_ASSERT(Removed == 1, "Failed to remove projectile (Removed={})", Removed);
		return (Removed == 1);
	}

	void CRifle::DestroyExpiredProjectiles()
	{
		/* Remove expired projectiles. */
		while (!ExpiredQueue.empty()) {
			b2BodyId& Expired = ExpiredQueue.front();
			DestroyProjectile(Expired);
			ExpiredQueue.pop();
		}
	}

}
