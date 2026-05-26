#pragma once

#include <box2d/box2d.h>

#include "core/core.h"
#include "renderer/color.h"
#include "scene/actor.h"

namespace platformer2d {

	class CProjectileSystem;

	class CProjectile : public CActor
	{
	public:
		CProjectile(const FActorSpecification& InSpec, CActor* InSpawner);
		CProjectile() = delete;
		~CProjectile() = default;

		[[nodiscard]] bool ExplodesOnImpact() const { return bExplodeOnImpact; }
		[[nodiscard]] float GetDamage() const { return Damage; }
		[[nodiscard]] const CActor* GetSpawner() const { return Spawner; }
		[[nodiscard]] virtual EActorType GetActorType() const override { return EActorType::Projectile; }

	public:
		std::uint8_t BounceCount = 0;
		std::uint8_t MaxBounceCount = 1;

	private:
		CActor* Spawner = nullptr;
		b2BodyId ID{};
		b2ShapeId ShapeID{};
		std::chrono::steady_clock::time_point TimeFired;
		std::chrono::milliseconds ExpireTimeout = 3000ms;

		bool bExplodeOnImpact = true;
		float Damage = 10.0f;
		float Radius = 0.040f;
		float RenderZ = -0.020f;

		friend class CProjectileSystem;

		LK_CLASS();
	};

}
