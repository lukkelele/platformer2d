#include "gameplaysystem.h"

#include "scene/actor.h"

namespace platformer2d {

	bool CGameplaySystem::Teleport(std::shared_ptr<CActor> Source, const glm::vec2& Destination)
	{
		if (!Source)
		{
			return false;
		}

		LK_INFO_TAG("GameplaySystem", "Teleport \"{}\": {} -> {}", Source->GetName(), Source->GetPosition(), Destination);
		Source->SetPosition(Destination);
	}

	bool CGameplaySystem::Teleport(std::shared_ptr<CActor> Source, std::shared_ptr<CActor> Destination)
	{
		if (!Source || !Destination)
		{
			return false;
		}

		LK_INFO_TAG("GameplaySystem", "Teleport \"{}\": {} -> {}", Source->GetName(), Source->GetPosition(), Destination->GetPosition());
		Source->SetPosition(Destination->GetPosition());
	}

}

