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

		/**
		 * @brief Fire at target position in world space.
		 */
		void Fire(const glm::vec2& TargetPos);

		bool Reload();
		bool NeedToReload() const { return Ammo <= 0; }

		void Equip(CActor* Actor);

		/**
		 * @brief Check if the rifle is held by a specific actor.
		 */
		bool IsHeldBy(const CActor* Actor) const;

		void SetProjectileRadius(float InRadius);
		float GetProjectileRadius() const { return ProjectileRadius; }
		void SetProjectileVelocity(float InVelocity);
		float GetProjectileVelocity() const { return ProjectileVelocity; }
		void SetProjectileExplodeOnImpact(bool ExplodeOnImpact);
		float GetProjectileExplodeOnImpact() const { return bProjectileExplodeOnImpact; }
		void SetProjectileColor(const glm::vec4& InColor);
		const glm::vec4& GetProjectileColor() const { return ProjectileColor; }

		const CActor* GetOwner() const { return Owner; }
		virtual EWeaponType GetType() const override { return EWeaponType::Rifle; }

	private:
		bool DestroyProjectile(const b2BodyId& ID);

	public:
		static constexpr uint16_t MAGAZINE_SIZE = 30;
	private:
		CActor* Owner = nullptr;
		glm::vec2 Origin{};
		std::chrono::milliseconds ExpireTimeout = 1000ms;

		float ProjectileVelocity = 5.0f; /* Base velocity in a single axis. */
		float ProjectileRadius = 0.050f;
		glm::vec4 ProjectileColor = FColor::Red;
		bool bProjectileExplodeOnImpact = true;
		glm::vec2 MuzzleOffset = { 0.050f, 0.0f };

		uint16_t Ammo = MAGAZINE_SIZE;

		std::vector<std::shared_ptr<CProjectile>> Fired{};
		std::queue<b2BodyId> ExpiredQueue{};
	};

}
