#pragma once

#include <queue>

#include "weapon.h"
#include "projectile.h"

namespace platformer2d {

	class CRifle : public IWeapon
	{
	public:
		CRifle();
		~CRifle();

		virtual void Tick() override;
		virtual void Render() override;

		/**
		 * @brief Fire at target position in world space.
		 */
		void Fire(const glm::vec2& TargetPos);

		bool Reload();
		bool NeedToReload() const { return Ammo <= 0; }
		uint16_t GetAmmo() const { return Ammo; }

		void Equip(CActor* Actor);

		/**
		 * @brief Check if the rifle is held by a specific actor.
		 */
		bool IsHeldBy(const CActor* Actor) const;

		/**
		 * @brief Enable or disable if the rifle can shoot.
		 */
		void SetEnabled(bool Enabled);
		bool IsEnabled() const { return bEnabled; }

		void SetLookDirection(EDirection InDirection);
		EDirection GetLookDirection() const { return LookDir; }
		void RequestLookDirection(EDirection InDirection);

		void SetProjectileRadius(float InRadius);
		float GetProjectileRadius() const { return ProjectileRadius; }
		void SetProjectileVelocity(float InVelocity);
		float GetProjectileVelocity() const { return ProjectileVelocity; }
		void SetProjectileRestitution(float InRestitution);
		float GetProjectileRestitution() const { return ProjectileRestitution; }
		void SetProjectileExplodeOnImpact(bool ExplodeOnImpact);
		float GetProjectileExplodeOnImpact() const { return bProjectileExplodeOnImpact; }
		void SetProjectileColor(const glm::vec4& InColor);
		const glm::vec4& GetProjectileColor() const { return ProjectileColor; }

		const CActor* GetOwner() const { return Owner; }
		virtual EWeaponType GetWeaponType() const override { return EWeaponType::Rifle; }

	private:
		bool DestroyProjectile(const b2BodyId& ID);
		void DestroyExpiredProjectiles();

	public:
		static constexpr uint16_t MAGAZINE_SIZE = 30;
	private:
		CActor* Owner = nullptr;
		glm::vec3 Origin{ 0.0f, 0.0f, -0.10f };
		std::chrono::milliseconds ExpireTimeout = 1000ms;
		EDirection LookDir = EDirection::Right;

		/** Shooting enabled/disabled. */
		bool bEnabled = true;

		float ProjectileVelocity = 9.0f; /* Base velocity in a single axis. */
		float ProjectileRestitution = 0.25f;
		float ProjectileRadius = 0.030f;
		glm::vec4 ProjectileColor = FColor::Black;
		/**
		 * @todo: Need to add actor flag to check if hit actors are damageable
		 * to allow this property to be set to true. Or else the bounced projectiles
		 * will not work.
		 */
		bool bProjectileExplodeOnImpact = false;
		uint8_t ProjectileBounceCount = 3;

		glm::vec2 MuzzleOffset = { 0.080f, 0.050f };
		uint16_t Ammo = MAGAZINE_SIZE;

		std::vector<std::shared_ptr<CProjectile>> Fired{};
		std::queue<b2BodyId> ExpiredQueue{};
	};

}
