#include "instance.h"

#include "core/window.h"
#include "game/checkpointsystem.h"
#include "game/healthsystem.h"
#include "game/projectile.h"
#include "game/rifle.h"
#include "physics/body.h"
#include "scene/scene.h"

namespace platformer2d {

	CGameInstance::CGameInstance(CGameInstance* InstanceRef, const FGameSpecification& InSpec)
		: CLayer(InSpec.InstanceName)
		, Spec(InSpec)
	{
		Instance = InstanceRef;
		LK_VERIFY(Instance, "Invalid game instance reference");

		UpdateViewportBounds();
	}

	CGameInstance::~CGameInstance()
	{
		Instance = nullptr;
	}

	glm::vec2 CGameInstance::GetMouseInViewportSpace()
	{
		auto [MouseX, MouseY] = CMouse::GetPos();
		MouseX -= ViewportBounds[0].x;
		MouseY -= ViewportBounds[0].y;
		const float ViewportWidth = ViewportBounds[1].x - ViewportBounds[0].x;
		const float ViewportHeight = ViewportBounds[1].y - ViewportBounds[0].y;

		return glm::vec2(
			(MouseX / static_cast<float>(ViewportWidth)) * 2.0f - 1.0f,
			((MouseY / static_cast<float>(ViewportHeight)) * 2.0f - 1.0f) * -1.0f);
	}

	glm::vec2 CGameInstance::GetMouseInWorldSpace(const CCamera& Camera)
	{
		const glm::vec2 MousePos = GetMouseInViewportSpace();
		if ((MousePos.x < -1.0f) || (MousePos.x > 1.0f) || (MousePos.y < -1.0f) || (MousePos.y > 1.0f)) {
			return glm::vec2(std::numeric_limits<float>::quiet_NaN());
		}

		const glm::vec4 ClipPos = glm::vec4(MousePos.x, MousePos.y, 0.0f, 1.0f);
		const glm::mat4 InvViewProj = glm::inverse(Camera.GetProjectionMatrix() * Camera.GetViewMatrix());
		glm::vec4 WorldPos = InvViewProj * ClipPos;
		if (WorldPos.w != 0.0f) {
			WorldPos /= WorldPos.w;
		}

		return WorldPos;
	}

	void CGameInstance::UpdateViewportBounds()
	{
		ViewportBounds[0] = {0.0f, 0.0f};
		ViewportBounds[1] = CWindow::Get().GetSize();
	}

	void CGameInstance::BindPhysicsEvents()
	{
		DelegateHandles.OnSensorBeginEvent = CPhysicsWorld::OnSensorBeginEvent.Add(this, &CGameInstance::OnSensorBeginEvent);
		DelegateHandles.OnSensorEndEvent = CPhysicsWorld::OnSensorEndEvent.Add(this, &CGameInstance::OnSensorEndEvent);
		DelegateHandles.OnContactBeginEvent = CPhysicsWorld::OnContactBeginEvent.Add(this, &CGameInstance::OnContactBeginEvent);
		DelegateHandles.OnContactEndEvent = CPhysicsWorld::OnContactEndEvent.Add(this, &CGameInstance::OnContactEndEvent);
	}

	void CGameInstance::UnbindPhysicsEvents()
	{
		CPhysicsWorld::OnSensorBeginEvent.Remove(DelegateHandles.OnSensorBeginEvent);
		CPhysicsWorld::OnSensorEndEvent.Remove(DelegateHandles.OnSensorEndEvent);
		CPhysicsWorld::OnContactBeginEvent.Remove(DelegateHandles.OnContactBeginEvent);
		CPhysicsWorld::OnContactEndEvent.Remove(DelegateHandles.OnContactEndEvent);
	}

	bool CGameInstance::PreSolve(b2ShapeId ShapeA, b2ShapeId ShapeB, b2Vec2 Point, b2Vec2 Normal, void* Ctx)
	{
		LK_ASSERT(b2Shape_IsValid(ShapeA) && b2Shape_IsValid(ShapeB));
		if (!Ctx) {
			return false;
		}

		CPlayer& Player = *static_cast<CPlayer*>(Ctx);
		const b2ShapeId PlayerShapeID = Player.GetBody()->GetShapeID();

		const bool InvolvesPlayer = B2_ID_EQUALS(ShapeA, PlayerShapeID) || B2_ID_EQUALS(ShapeB, PlayerShapeID);
		if (!InvolvesPlayer) {
			return true;
		}

		if (B2_ID_EQUALS(ShapeA, PlayerShapeID)) {
			Normal.x = -Normal.x;
			Normal.y = -Normal.y;
		}

		const b2Vec2 Up = {0.0f, 1.0f};
		const float UpDot = Normal.x * Up.x + Normal.y * Up.y;
		if (UpDot <= 0.0f) {
			return true;
		}

		const b2BodyId PlayerBody = Player.GetBody()->GetID();
		const b2Vec2 V = b2Body_GetLinearVelocity(PlayerBody);
		const float Vn = V.x * Normal.x + V.y * Normal.y;
		if (Vn > 0.0f) {
			return false;
		}

		return true;
	}

	void CGameInstance::OnSensorBeginEvent(const CSensorBeginEvent& Event)
	{
		LK_ASSERT(Event.Sensor && Event.Visitor);
		LK_DEBUG_TAG("GameInstance", "OnSensorBeginEvent: Sensor={} Visitor={}", Event.Sensor->GetName(), Event.Visitor->GetName());
		const std::shared_ptr<CPlayer> P = GetPlayer(0);
		if (!P || (Event.Sensor != P.get()) && (Event.Visitor != P.get())) {
			return;
		}

		if (Event.Visitor == P.get()) {
			if (auto* IC = Event.Sensor->TryGetComponent<FInteractionComponent>()) {
				LK_DEBUG("[BEGIN] Interaction: {}", Enum::ToString(IC->GetType()));
				Event.Sensor->SetOutlineEnabled(true);

				std::visit([&](auto&& Data)
				{
					using T = std::decay_t<decltype(Data)>;
					if constexpr (std::is_same_v<T, FDamageInteraction>) {
						CHealthSystem::ApplyDamage(Event.Visitor, Data.Damage);
					} else if constexpr (std::is_same_v<T, FPickupInteraction>) {
						CPlayer& PlayerRef = *static_cast<CPlayer*>(Event.Visitor);
						OnPickupEvent(PlayerRef, *IC);
					} else if constexpr (std::is_same_v<T, FHealInteraction>) {
						CHealthSystem::Heal(Event.Visitor, Data.Amount);
						if (Data.bConsumeOnUse) {
							LK_DEBUG_TAG("GameInstance", "[TODO] Despawn heal source {}", Event.Sensor->GetName());
						}
					} else if constexpr (std::is_same_v<T, FKillzoneInteraction>) {
						CHealthSystem::Kill(Event.Visitor);
					} else if constexpr (std::is_same_v<T, FJumppadInteraction>) {
						if (CBody* B = Event.Visitor->GetBody()) {
							const glm::vec2 Vel = B->GetLinearVelocity();
							const float NewX = Data.bPreserveHorizontalVelocity ? Vel.x : 0.0f;
							B->SetLinearVelocity({NewX, Data.Impulse.y});
						}
					} else if constexpr (std::is_same_v<T, FClimbableInteraction>) {
						static_cast<CPlayer*>(Event.Visitor)->SetClimbZone(true, Data.ClimbSpeed);
					} else if constexpr (std::is_same_v<T, FCheckpointInteraction>) {
						CPlayer& PlayerRef = *static_cast<CPlayer*>(Event.Visitor);
						const std::shared_ptr<CScene> S = GetScene();
						const std::filesystem::path ScenePath = S ? S->GetFilepath() : LastSceneFilepath;
						CCheckpointSystem::TrySave(PlayerRef, Data.CheckpointID, ScenePath);
					}
				}, IC->GetData());
			}
		}
	}

	void CGameInstance::OnSensorEndEvent(const CSensorEndEvent& Event)
	{
		LK_ASSERT(Event.Sensor && Event.Visitor);
		LK_DEBUG_TAG("GameInstance", "OnSensorEndEvent: Sensor={} Visitor={}", Event.Sensor->GetName(), Event.Visitor->GetName());
		const std::shared_ptr<CPlayer> P = GetPlayer(0);
		if (!P || (Event.Sensor != P.get()) && (Event.Visitor != P.get())) {
			return;
		}

		if (Event.Visitor == P.get()) {
			if (auto* IC = Event.Sensor->TryGetComponent<FInteractionComponent>()) {
				LK_DEBUG("[END] Interaction: {}", Enum::ToString(IC->GetType()));
				Event.Sensor->SetOutlineEnabled(false);

				if (IC->GetType() == EInteraction::Climbable) {
					static_cast<CPlayer*>(Event.Visitor)->SetClimbZone(false);
				}
			}
		}
	}

	static void OnProjectileContact(CActor* ProjectileActor, CActor* HitActor)
	{
		CProjectile* Projectile = static_cast<CProjectile*>(ProjectileActor);
		if (Projectile->GetOwner() && Projectile->GetOwner()->IsHeldBy(HitActor)) {
			return;
		}

		Projectile->BounceCount++;

		LK_ASSERT(HitActor, "Invalid projectile hit");
		LK_TRACE("{}: Hit: {} ({})", ProjectileActor->GetName(), HitActor->GetName(), Enum::ToString(HitActor->GetActorType()));
		CHealthSystem::ApplyDamage(HitActor, Projectile->GetDamage());

		if (Projectile->ExplodesOnImpact()) {
			Projectile->Destroy();
		} else if (Projectile->BounceCount >= Projectile->MaxBounceCount) {
			LK_TRACE("{}: Max bounce reached: {}", Projectile->GetName(), Projectile->BounceCount);
			Projectile->Destroy();
		}
	}

	void CGameInstance::OnContactBeginEvent(const CContactBeginEvent& Event)
	{
		LK_TRACE_TAG("GameInstance", "OnContactBeginEvent: A={} B={}", (Event.A ? Event.A->GetName() : "NULL"), (Event.B ? Event.B->GetName() : "NULL"));
		LK_ASSERT(Event.A && Event.B, "Invalid event references");
		if (!Event.A || !Event.B) {
			return;
		}

		const EActorType AType = Event.A->GetActorType();
		const EActorType BType = Event.B->GetActorType();

		if (AType == EActorType::Projectile) {
			OnProjectileContact(Event.A, Event.B);
		} else if (BType == EActorType::Projectile) {
			OnProjectileContact(Event.B, Event.A);
		}
	}

	void CGameInstance::OnContactEndEvent(const CContactEndEvent& Event)
	{
		LK_TRACE_TAG("GameInstance", "OnContactEndEvent: A={} B={}", (Event.A ? Event.A->GetName() : "NULL"), (Event.B ? Event.B->GetName() : "NULL"));
		LK_ASSERT(Event.A && Event.B, "Invalid event references");
		if (!Event.A || !Event.B) {
			return;
		}
	}

	void CGameInstance::OnPickupEvent(CPlayer& InPlayer, const FInteractionComponent& IC)
	{
		const auto& Data = std::get<FPickupInteraction>(IC.GetData());
		switch (Data.Kind) {
			case EPickupKind::Item:
				OnPickupEvent_Item(Data, InPlayer);
				break;
			case EPickupKind::Weapon:
				OnPickupEvent_Rifle(Data, InPlayer);
				break;
		}
	}

	void CGameInstance::OnPickupEvent_Item(const FPickupInteraction& Interaction, CPlayer& InPlayer)
	{
		const auto& Object = std::get<FPickupItem>(Interaction.Object);
		LK_WARN("Item={} ExpireOnPickup={}", Enum::ToString(Object.Type), Interaction.bExpireWhenPickedUp);
	}

	void CGameInstance::OnPickupEvent_Rifle(const FPickupInteraction& Interaction, CPlayer& InPlayer)
	{
		const auto& Object = std::get<FPickupWeapon>(Interaction.Object);
		const auto& Spec = std::get<FRifleSpecification>(Object.Spec);
		LK_TRACE("Pickup Weapon={} MagazineSize={} ExpireOnPickup={}", Enum::ToString(Object.Type), Spec.MagazineSize, Interaction.bExpireWhenPickedUp);
		CInventory& Inventory = InPlayer.GetInventory();
		if (Inventory.IsEmpty()) {
			std::shared_ptr<CRifle> Rifle = std::make_shared<CRifle>(Spec, &InPlayer);
			Inventory.AddItem(Rifle);
		} else {
			LK_WARN_TAG("GameInstance", "Inventory not empty");
		}
	}

}

