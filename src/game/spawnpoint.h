#pragma once

#include "core/core.h"
#include "scene/actor.h"

namespace platformer2d {

	class CSpawnpoint : public CActor
	{
	public:
		CSpawnpoint(const glm::vec2& InPos = { 0.0f, 0.0f });
		~CSpawnpoint() = default;

		virtual EActorType GetType() const override { return EActorType::Spawnpoint; }
	};

}
