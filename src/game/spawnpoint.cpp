#include "spawnpoint.h"

namespace platformer2d {

	namespace {
		const FBodySpecification DefaultBodySpec = {
			.bSensor = true,
			.Flags = EBodyFlag_SensorEvents
		};
	}

	CSpawnpoint::CSpawnpoint(const glm::vec2& InPos)
		: CActor(FActorSpecification(), DefaultBodySpec)
	{
		LK_DEBUG_TAG("Spawnpoint", "Created at {}", InPos);
		SetPosition(InPos);
	}

}
