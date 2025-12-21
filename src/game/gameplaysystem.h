#pragma once

#include "core/core.h"

namespace platformer2d {

	class CActor;

	class CGameplaySystem
	{
	public:
		CGameplaySystem() = delete;
		~CGameplaySystem() = delete;
		CGameplaySystem(CGameplaySystem&&) = delete;
		CGameplaySystem(const CGameplaySystem&) = delete;

		static bool Teleport(CActor* Source, const glm::vec2& Destination);
		static bool Teleport(std::shared_ptr<CActor> Source, const glm::vec2& Destination);
		static bool Teleport(std::shared_ptr<CActor> Source, std::shared_ptr<CActor> Destination);
	};

}

