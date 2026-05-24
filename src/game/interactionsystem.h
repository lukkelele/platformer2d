#pragma once

#include "core/core.h"
#include "core/delegate.h"
#include "gamesystem.h"
#include "physics/events.h"
#include "scene/components.h"

namespace platformer2d {

	class CPlayer;

	class CInteractionSystem : public IGameSystem
	{
	public:
		CInteractionSystem() = default;
		CInteractionSystem(CInteractionSystem&&) = delete;
		CInteractionSystem(const CInteractionSystem&) = delete;
		~CInteractionSystem() = default;

		CInteractionSystem& operator=(CInteractionSystem&&) = delete;
		CInteractionSystem& operator=(const CInteractionSystem&) = delete;

		void Initialize(CGameInstance& Owner) override;
		void Shutdown() override;
		void Tick() override;

	protected:
		virtual void OnSensorBegin(const CSensorBeginEvent& Event);
		virtual void OnSensorEnd(const CSensorEndEvent& Event);

		virtual void HandlePickup(CPlayer& Player, const FInteractionComponent& IC);
		virtual void HandlePickup_Item(const FPickupInteraction& Interaction, CPlayer& Player);
		virtual void HandlePickup_Weapon(const FPickupInteraction& Interaction, CPlayer& Player);

	private:
		std::vector<TInteractionData*> ActiveInteractions;
		//std::vector<TInteractionData&> ActiveInteractions;

		Core::FDelegateHandle OnSensorBeginHandle;
		Core::FDelegateHandle OnSensorEndHandle;
	};

}

