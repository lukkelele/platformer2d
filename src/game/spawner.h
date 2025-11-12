#pragma once

#include "core/core.h"
#include "renderer/color.h"
#include "scene/actor.h"
#include "physics/body.h"

namespace platformer2d {

	class CSpawner
	{
	public:
		CSpawner() = default;
		~CSpawner() = default;
		CSpawner(const CSpawner&) = delete;
		CSpawner(CSpawner&&) = delete;

		static std::shared_ptr<CActor> CreateStaticPolygon(std::string_view Name, const glm::vec2& Pos,
														   const glm::vec2& Size, const glm::vec4& Color = FColor::White);

	};

}
