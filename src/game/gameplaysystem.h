#pragma once

#include "core/core.h"
#include "gamesystem.h"

namespace platformer2d {

	class CActor;

	/* @todo: Rename this class to something else */
	class CGameplaySystem : public IGameSystem
	{
	public:
		CGameplaySystem() = default;
		CGameplaySystem(CGameplaySystem&&) = delete;
		CGameplaySystem(const CGameplaySystem&) = delete;
		~CGameplaySystem() = default;

		CGameplaySystem& operator=(CGameplaySystem&&) = delete;
		CGameplaySystem& operator=(const CGameplaySystem&) = delete;

		void Initialize(CGameInstance& Owner) override {}
		void Shutdown() override {}

		bool Teleport(CActor* Source, const glm::vec2& Destination);
		bool Teleport(std::shared_ptr<CActor> Source, const glm::vec2& Destination);
		bool Teleport(std::shared_ptr<CActor> Source, std::shared_ptr<CActor> Destination);
	};

}

