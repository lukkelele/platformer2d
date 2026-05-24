#include "projectile.h"

namespace platformer2d {

	CProjectile::CProjectile(const FActorSpecification& InSpec, CActor* InSpawner)
		: CActor(InSpec)
		, Spawner(InSpawner)
	{
	}

}
