#include "projectile.h"

namespace platformer2d {

	CProjectile::CProjectile(const FActorSpecification& InSpec, CRifle* InOwner, const TDestroy DestroyCallback)
		: CActor(InSpec)
		, Owner(InOwner)
		, OnDestroy(DestroyCallback)
	{
	}

	void CProjectile::Destroy()
	{
		LK_TRACE("{}: Destroy", Name);
		if (Owner && OnDestroy) {
			(Owner->*OnDestroy)(ID);
		}
	}

}
