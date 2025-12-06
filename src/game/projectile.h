#pragma once

#include <box2d/box2d.h>

#include "core/core.h"
#include "renderer/color.h"
#include "scene/actor.h"

namespace platformer2d {

	class CRifle;

	class CProjectile : public CActor
	{
	public:
		using TDestroy = bool(CRifle::*)(const b2BodyId&);
	public:
		CProjectile(const FActorSpecification& InSpec, CRifle* InOwner, TDestroy DestroyCallback);
		CProjectile() = delete;
		~CProjectile() = default;

		void Destroy();
		bool ExplodesOnImpact() const { return bExplodeOnImpact; }

		const CRifle* GetOwner() const { return Owner; }
		virtual EActorType GetActorType() const override { return EActorType::Projectile; }

	private:
		CRifle* Owner;
		TDestroy OnDestroy;
		b2BodyId ID{};
		b2ShapeId ShapeID{};
		std::chrono::steady_clock::time_point TimeFired;

		bool bExplodeOnImpact = true;

		friend class CRifle;
	};

}
