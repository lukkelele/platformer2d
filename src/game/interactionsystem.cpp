#include "interactionsystem.h"

#include "checkpointsystem.h"
#include "healthsystem.h"
#include "instance.h"
#include "inventory.h"
#include "player.h"
#include "rifle.h"
#include "physics/body.h"
#include "physics/physicsworld.h"
#include "scene/actor.h"
#include "scene/scene.h"

namespace platformer2d {

	void CInteractionSystem::Initialize(CGameInstance& Owner)
	{
		LK_DEBUG_TAG("InteractionSystem", "Initialize");
		OwnerRef = &Owner;
		OnSensorBeginHandle = CPhysicsWorld::OnSensorBeginEvent.Add(this, &CInteractionSystem::OnSensorBegin);
		OnSensorEndHandle = CPhysicsWorld::OnSensorEndEvent.Add(this, &CInteractionSystem::OnSensorEnd);
	}

	void CInteractionSystem::Shutdown()
	{
		LK_DEBUG_TAG("InteractionSystem", "Shutdown");
		CPhysicsWorld::OnSensorBeginEvent.Remove(OnSensorBeginHandle);
		CPhysicsWorld::OnSensorEndEvent.Remove(OnSensorEndHandle);
		OwnerRef = nullptr;
	}

	void CInteractionSystem::OnSensorBegin(const CSensorBeginEvent& Event)
	{
		LK_ASSERT(Event.Sensor && Event.Visitor);
		LK_DEBUG_TAG("Interaction", "OnSensorBegin: Sensor={} Visitor={}", Event.Sensor->GetName(), Event.Visitor->GetName());
		LK_ASSERT(OwnerRef);
		/* Force a sensor event to contain a player. This will probably change in the future. */
		const std::shared_ptr<CPlayer> Player = OwnerRef->GetPlayer(0);
		if (!Player || (Event.Sensor != Player.get()) && (Event.Visitor != Player.get())) {
			return;
		}

		auto* IC = Event.Sensor->TryGetComponent<FInteractionComponent>();
		if (!IC) {
			return;
		}

		LK_DEBUG("[BEGIN] Interaction: {}", Enum::ToString(IC->GetType()));
		Event.Sensor->SetOutlineEnabled(true);

		std::visit([&]<typename T>(T& Data)
		{
			if constexpr (std::is_same_v<T, FDamageInteraction>) {
				OwnerRef->GetSystem<CHealthSystem>().ApplyDamage(Event.Visitor, Data.Damage);
			} else if constexpr (std::is_same_v<T, FPickupInteraction>) {
				if (Event.Visitor == Player.get()) {
					CPlayer& PlayerRef = *static_cast<CPlayer*>(Event.Visitor);
					HandlePickup(PlayerRef, *IC);
				}
			} else if constexpr (std::is_same_v<T, FHealInteraction>) {
				OwnerRef->GetSystem<CHealthSystem>().Heal(Event.Visitor, Data.Amount);
				if (Data.bConsumeOnUse) {
					LK_DEBUG_TAG("Interaction", "[TODO] Despawn heal source {}", Event.Sensor->GetName());
				}
			} else if constexpr (std::is_same_v<T, FKillzoneInteraction>) {
				OwnerRef->GetSystem<CHealthSystem>().Kill(Event.Visitor);
			} else if constexpr (std::is_same_v<T, FJumppadInteraction>) {
				if (CBody* Body = Event.Visitor->GetBody()) {
					const glm::vec2 Vel = Body->GetLinearVelocity();
					const float NewX = Data.bPreserveHorizontalVelocity ? Vel.x : 0.0f;
					Body->SetLinearVelocity({NewX, Data.Impulse.y});
				}
			} else if constexpr (std::is_same_v<T, FClimbableInteraction>) {
				if (Event.Visitor == Player.get()) {
					static_cast<CPlayer*>(Event.Visitor)->SetClimbZone(true, Data.ClimbSpeed);
				}
			} else if constexpr (std::is_same_v<T, FCheckpointInteraction>) {
				if (Event.Visitor == Player.get()) {
					CPlayer& PlayerRef = *static_cast<CPlayer*>(Event.Visitor);
					const std::shared_ptr<CScene> Scene = OwnerRef->GetScene();
					const std::filesystem::path ScenePath = (Scene ? Scene->GetFilepath() : OwnerRef->GetLastSceneFilepath());
					OwnerRef->GetSystem<CCheckpointSystem>().TrySave(PlayerRef, Data.CheckpointID, ScenePath);
				}
			}
		}, IC->GetData());
	}

	void CInteractionSystem::OnSensorEnd(const CSensorEndEvent& Event)
	{
		LK_ASSERT(Event.Sensor && Event.Visitor);
		LK_DEBUG_TAG("Interaction", "OnSensorEnd: Sensor={} Visitor={}", Event.Sensor->GetName(), Event.Visitor->GetName());
		LK_ASSERT(OwnerRef);
		/* Force a sensor event to contain a player. This will probably change in the future. */
		const std::shared_ptr<CPlayer> Player = OwnerRef->GetPlayer(0);
		if (!Player || (Event.Sensor != Player.get()) && (Event.Visitor != Player.get())) {
			return;
		}

		auto* IC = Event.Sensor->TryGetComponent<FInteractionComponent>();
		LK_ASSERT(IC, R"(Actor "{}" is missing an interaction component)", Event.Sensor->GetName());
		if (!IC) {
			return;
		}

		LK_DEBUG("[END] Interaction: {}", Enum::ToString(IC->GetType()));
		Event.Sensor->SetOutlineEnabled(false);

		if (IC->GetType() == EInteraction::Climbable) {
			if (Event.Visitor == Player.get()) {
				static_cast<CPlayer*>(Event.Visitor)->SetClimbZone(false);
			}
		}
	}

	void CInteractionSystem::HandlePickup(CPlayer& Player, const FInteractionComponent& IC)
	{
		const auto& Data = std::get<FPickupInteraction>(IC.GetData());
		switch (Data.Kind) {
			case EPickupKind::Item:
				HandlePickup_Item(Data, Player);
				break;
			case EPickupKind::Weapon:
				HandlePickup_Weapon(Data, Player);
				break;
		}
	}

	void CInteractionSystem::HandlePickup_Item(const FPickupInteraction& Interaction, CPlayer& Player)
	{
		const auto& Object = std::get<FPickupItem>(Interaction.Object);
		LK_WARN("Item={} ExpireOnPickup={}", Enum::ToString(Object.Type), Interaction.bExpireWhenPickedUp);
	}

	void CInteractionSystem::HandlePickup_Weapon(const FPickupInteraction& Interaction, CPlayer& Player)
	{
		const auto& Object = std::get<FPickupWeapon>(Interaction.Object);
		const auto& Spec = std::get<FRifleSpecification>(Object.Spec);
		LK_TRACE("Pickup Weapon={} MagazineSize={} ExpireOnPickup={}", Enum::ToString(Object.Type), Spec.MagazineSize, Interaction.bExpireWhenPickedUp);
		CInventory& Inventory = Player.GetInventory();
		if (Inventory.IsEmpty()) {
			std::shared_ptr<CRifle> Rifle = std::make_shared<CRifle>(Spec, &Player);
			Inventory.AddItem(Rifle);
		} else {
			LK_WARN_TAG("Interaction", "Inventory not empty");
		}
	}

}
