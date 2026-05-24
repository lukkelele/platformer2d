#pragma once

#include <queue>

#include "weapon.h"
#include "projectile.h"

namespace platformer2d {

	class CRifle : public IWeapon
	{
	public:
		CRifle(const FRifleSpecification& InSpec = FRifleSpecification(), CActor* InOwner = nullptr);
		~CRifle();

		void Tick(float DeltaTime) override;
		void Render() override;
		[[nodiscard]] EWeaponType GetWeaponType() const override { return EWeaponType::Rifle; }

		void PrimaryAction(const glm::vec2& TargetWorldPos) override;

		/**
		 * @brief Fire at target position in world space.
		 */
		void Fire(const glm::vec2& TargetPos);

		bool Reload();
		[[nodiscard]] bool NeedToReload() const { return Ammo <= 0; }
		[[nodiscard]] std::uint16_t GetAmmo() const { return Ammo; }
		[[nodiscard]] std::uint16_t GetMagazineSize() const { return MagazineSize; }

		void Equip(CActor* Actor);

		/**
		 * @brief Check if the rifle is held by a specific actor.
		 */
		[[nodiscard]] bool IsHeldBy(const CActor* Actor) const;

		/**
		 * @brief Enable or disable if the rifle can shoot.
		 */
		void SetEnabled(bool Enabled);
		bool IsEnabled() const { return bEnabled; }

		void SetLookDirection(EDirection InDirection);
		[[nodiscard]] EDirection GetLookDirection() const { return LookDir; }
		void RequestLookDirection(EDirection InDirection);

		void SetProjectileRadius(float InRadius);
		[[nodiscard]] float GetProjectileRadius() const { return ProjectileRadius; }
		void SetProjectileVelocity(float InVelocity);
		[[nodiscard]] float GetProjectileVelocity() const { return ProjectileVelocity; }
		void SetProjectileRestitution(float InRestitution);
		[[nodiscard]] float GetProjectileRestitution() const { return ProjectileRestitution; }
		void SetProjectileExplodeOnImpact(bool ExplodeOnImpact);
		[[nodiscard]] float GetProjectileExplodeOnImpact() const { return bProjectileExplodeOnImpact; }
		void SetProjectileColor(const glm::vec4& InColor);
		[[nodiscard]] const glm::vec4& GetProjectileColor() const { return ProjectileColor; }

		[[nodiscard]] const CActor* GetOwner() const { return Owner; }

	private:
		CActor* Owner = nullptr;
		glm::vec3 Origin{0.0f, 0.0f, -0.10f};
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
		std::uint8_t ProjectileBounceCount = 3;
		float ProjectileDamage = 10.0f;

		std::uint16_t Ammo;
		std::uint16_t MagazineSize;
		glm::vec2 MuzzleOffset = {0.080f, 0.050f};
	};

}
