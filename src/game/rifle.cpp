#include "rifle.h"

#include "game/player.h"
#include "game/instance.h"
#include "game/projectilesystem.h"
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
	}

	void CRifle::Tick(const float DeltaTime)
	{
		if (Owner) {
			Origin = Owner->GetPosition();
		}

		Render();
	}

	void CRifle::Render()
	{
		const float OffsetX = ((LookDir == EDirection::Left) ? -MuzzleOffset.x : MuzzleOffset.x);
		CRenderer::DrawQuad(
			glm::vec3(Origin.x + OffsetX + 0.0020f, Origin.y - MuzzleOffset.y - 0.0090f, -0.10f),
			glm::vec2(0.15f, 0.10f),
			*CRenderer::GetTexture(ETexture::Rifle),
			CRenderer::GetTextureCoords(LookDir));
	}

	void CRifle::PrimaryAction(const glm::vec2& TargetWorldPos)
	{
		Fire(TargetWorldPos);
	}

	void CRifle::Fire(const glm::vec2& TargetPos)
	{
		if (!Owner || (Ammo <= 0)) {
			return;
		}

		const glm::vec2 Diff = TargetPos - glm::vec2(Origin.x, Origin.y);
		float LenSq = (Diff.x * Diff.x) + (Diff.y * Diff.y);
		if (LenSq <= 0.0000010f) {
			return;
		}

		const float InvLen = (1.0f / std::sqrt(LenSq));
		const glm::vec2 Dir = glm::vec2(Diff.x * InvLen, Diff.y * InvLen);
		const glm::vec2 Velocity = glm::vec2(ProjectileVelocity * Dir.x, ProjectileVelocity * Dir.y);

		glm::vec2 SpawnPos = glm::vec2(Origin.x, Origin.y);
		/* Offset muzzle based on look direction. */
		if (Velocity.x < 0.0f) {
			/* Left */
			SpawnPos.x -= MuzzleOffset.x;
			RequestLookDirection(EDirection::Left);
		} else if (Velocity.x > 0.0f) {
			/* Right */
			SpawnPos.x += MuzzleOffset.x;
			RequestLookDirection(EDirection::Right);
		}

		FProjectileSpawnParams Params;
		Params.Spawner = Owner;
		Params.Position = SpawnPos;
		Params.Velocity = Velocity;
		Params.Radius = ProjectileRadius;
		Params.Restitution = ProjectileRestitution;
		Params.Damage = ProjectileDamage;
		Params.Color = ProjectileColor;
		Params.MaxBounceCount = ProjectileBounceCount;
		Params.bExplodeOnImpact = bProjectileExplodeOnImpact;
		Params.bIsBullet = true;
		Params.RenderZ = -0.010f;
		Params.ExpireTimeout = ExpireTimeout;
		Params.NamePrefix = "Projectile";

		const CProjectile* Projectile = CGameInstance::Get().GetSystem<CProjectileSystem>().Spawn(Params);

		Ammo--;
		LK_TRACE_TAG("Rifle", "Fire: {} Ammo={} Velocity={}", Projectile->GetName(), Ammo, Velocity);
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

}
