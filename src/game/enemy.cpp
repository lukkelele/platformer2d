#include "enemy.h"

#include "game/gameplaysystem.h"
#include "game/healthsystem.h"
#include "game/instance.h"
#include "game/controller/patrolcontroller.h"
#include "renderer/renderer.h"
#include "scene/effectmanager.h"

namespace platformer2d {

	CEnemy::CEnemy(const FEnemySpecification& InSpec, const FActorSpecification& InActorSpec, const FBodySpecification& InBodySpec)
		: CActor(InActorSpec, InBodySpec)
		, ArchetypeKind(InSpec.Archetype)
	{
		Data.State = EEnemyState::Idle;
		Data.LookDirection = EDirection::Right;

		const FEnemyArchetype& Archetype = GetEnemyArchetype(ArchetypeKind);
		MoveSpeed = Archetype.MoveSpeed;
		FHealthComponent& HC = AddComponent<FHealthComponent>();
		HC.SetMaxHealth(Archetype.MaxHealth);
		HC.SetHealth(Archetype.MaxHealth);

		std::string_view SpritePath;
		if (Texture == ETexture::Goblin) {
			SpritePath = TEXTURES_DIR "/sprites/Goblin.lsprite";
		}
		FSpriteReader Reader;
		std::optional<FSpriteSheet> LoadedSheet = Reader.Read(SpritePath);
		LK_ASSERT(LoadedSheet, "Failed to read: {}", SpritePath);
		SpriteSheet = std::move(*LoadedSheet);

		const FSpriteCoord InitialFrame = SpriteSheet.Get(ESpriteFrame::Idle).First();
		SpriteFrame.Current = InitialFrame;
		SpriteFrame.Next = InitialFrame;

		const glm::vec2 TilePos{InitialFrame.X, InitialFrame.Y};
		Sprite = std::make_unique<CSprite>(CRenderer::GetTexture(Texture), TilePos, SpriteSheet.TileSize);
		LK_DEBUG_TAG("Enemy", R"([{}] {} Texture="{}" TilePos={})", Name, Enum::ToString(ArchetypeKind), Enum::ToString(Texture), TilePos);

		SetSpawnPoint(InSpec.SpawnPoint);
		CGameInstance::Get().GetSystem<CGameplaySystem>().Teleport(this, InSpec.SpawnPoint);
	}

	CEnemy::~CEnemy()
	{
		LK_DEBUG_TAG("Enemy", "Release: {}", Name);
	}

	void CEnemy::Tick(const float DeltaTime)
	{
		CActor::Tick(DeltaTime);

		if (Controller) {
			Controller->Tick(*this, DeltaTime);
		}

		if (DeltaTime > 0.0f) {
			CheckCollisions();
			UpdateMovementState();
			if (bShouldUpdateSprite) {
				UpdateSprite();
			}
		}
	}

	void CEnemy::SetController(std::unique_ptr<IEnemyController> InController)
	{
		Controller = std::move(InController);
		if (Controller) {
			Controller->OnPossess(*this);
		}
	}

	void CEnemy::SetState(const EEnemyState InState)
	{
		Data.State = InState;
		LK_DEBUG_TAG("Enemy", "[{}] State={}", Name, Enum::ToString(Data.State));
	}

	void CEnemy::SetLookDirection(const EDirection InDirection)
	{
		if (Data.LookDirection == InDirection) {
			return;
		}
		Data.LookDirection = InDirection;
		SetSpriteTilePos(SpriteFrame.Current, true);
	}

	void CEnemy::SetMoveSpeed(const float InSpeed)
	{
		MoveSpeed = InSpeed;
	}

	void CEnemy::MoveLeft()
	{
		SetLookDirection(EDirection::Left);
		ApplyMoveVelocityX(-MoveSpeed);
	}

	void CEnemy::MoveRight()
	{
		SetLookDirection(EDirection::Right);
		ApplyMoveVelocityX(MoveSpeed);
	}

	void CEnemy::StopMovement()
	{
		ApplyMoveVelocityX(0.0f);
	}

	void CEnemy::Jump()
	{
		const FEnemyArchetype& Archetype = GetEnemyArchetype(ArchetypeKind);
		if (!Archetype.bCanJump || !Body || bJumping) {
			return;
		}

		bJumping = true;
		SetMovementState(EMovementState::Airborne);
		Body->ApplyImpulse({0.0f, Archetype.JumpImpulse});
	}

	void CEnemy::Kill()
	{
		CGameInstance::Get().GetSystem<CHealthSystem>().Kill(this);
	}

	void CEnemy::OnDeath()
	{
		LK_DEBUG_TAG("Enemy", "[{}] OnDeath", GetName());
		CEffectManager::Get().Play(EEffect::Swoosh, GetPosition(), 300ms);
		SetFlag(EActorFlag_Transparent, 1);
		if (Body) {
			Body->SetEnabled(false);
		}
	}

	void CEnemy::Revive(const EReviveVariant Variant)
	{
		LK_DEBUG_TAG("Enemy", "[{}] Revive: {}", GetName(), Enum::ToString(Variant));
		if (Variant == EReviveVariant::AtSpawn) {
			CGameInstance::Get().GetSystem<CGameplaySystem>().Teleport(this, Data.SpawnPoint);
		}

		auto& HC = GetComponent<FHealthComponent>();
		HC.Health = HC.GetMaxHealth();

		CEffectManager::Get().Play(EEffect::Swoosh, GetPosition(), 300ms);
		SetFlag(EActorFlag_Transparent, 0);
		if (Body) {
			Body->SetEnabled(true);
		}
	}

	bool CEnemy::IsDead() const
	{
		return GetComponent<FHealthComponent>().IsDead();
	}

	void CEnemy::SetSpawnPoint(const glm::vec2& InPoint)
	{
		LK_DEBUG_TAG("Enemy", "Spawn point: {}", InPoint);
		Data.SpawnPoint = InPoint;
	}

	bool CEnemy::Serialize(YAML::Emitter& Out, EExtendableSerializer Extendable) const
	{
		LK_TRACE_TAG("Enemy", "[{}] Serializing", GetName());
		CActor::Serialize(Out, EExtendableSerializer::Yes);

		Out << YAML::Key << "Controller";
		Out << YAML::BeginMap;
		const EControllerType ControllerType = Controller ? Controller->GetControllerType() : EControllerType::None;
		Out << YAML::Key << "ControllerType" << YAML::Value << std::to_underlying(ControllerType);
		if (ControllerType == EControllerType::Patrol) {
			auto* ControllerRef = static_cast<CPatrolController*>(GetController());
			Out << YAML::Key << "HalfDistance" << YAML::Value << ControllerRef->GetHalfDistance();
			Out << YAML::Key << "StartDelayInSeconds" << YAML::Value << ControllerRef->GetStartDelayInSeconds();
		}
		Out << YAML::EndMap; /* ~Controller */

		Out << YAML::Key << "SpawnPoint" << YAML::Value << Data.SpawnPoint;
		Out << YAML::Key << "Archetype" << YAML::Value << static_cast<std::size_t>(ArchetypeKind);
		Out << YAML::EndMap; /* ~Actor */

		return true;
	}

	void CEnemy::SetMovementState(const EMovementState State)
	{
		if (MovementState == State) {
			return;
		}

		MovementState = State;
	}

	void CEnemy::UpdateMovementState()
	{
		if (bJustLanded) {
			LK_TRACE_TAG("Enemy", "Just landed");
			/* The idle state will get evaluated to idle/running later. */
			SetMovementState(EMovementState::Idle);
			bJustLanded = false;
		}

		switch (MovementState) {
			case EMovementState::Idle:
				OnMovementState_Idle();
				break;

			case EMovementState::Running:
				OnMovementState_Running();
				break;

			case EMovementState::Airborne:
				OnMovementState_Airborne();
				break;
		}

		bShouldUpdateSprite = (SpriteFrame.Current != SpriteFrame.Next);
	}

	void CEnemy::OnMovementState_Idle()
	{
		LK_ASSERT(Body);
		const glm::vec2 LinearVelocity = Body->GetLinearVelocity();
		if (std::abs(LinearVelocity.x) > CBody::LINEAR_VELOCITY_X_EPSILON) {
			SetMovementState(EMovementState::Running);
			return;
		}

		const FSpriteAnimation* IdleAnim = SpriteSheet.Find(ESpriteFrame::Idle);
		LK_ASSERT(IdleAnim, "{}: Missing anim", GetName());
		const std::uint16_t FrameIndex = CRenderer::GetFrameIndex();
		SpriteFrame.Next = IdleAnim->GetFrame(FrameIndex);
	}

	void CEnemy::OnMovementState_Running()
	{
		LK_ASSERT(Body);
		const glm::vec2 LinearVelocity = Body->GetLinearVelocity();
		if (std::abs(LinearVelocity.x) < CBody::LINEAR_VELOCITY_X_EPSILON) {
			SetMovementState(EMovementState::Idle);
			const FSpriteAnimation* IdleAnim = SpriteSheet.Find(ESpriteFrame::Idle);
			LK_ASSERT(IdleAnim, "{}: Missing anim", GetName());
			SpriteFrame.Next = IdleAnim->First();
			return;
		}

		const FSpriteAnimation* WalkAnim = SpriteSheet.Find(ESpriteFrame::Walk);
		LK_ASSERT(WalkAnim, "{}: Missing anim", GetName());
		const std::uint16_t FrameIndex = CRenderer::GetFrameIndex();
		SpriteFrame.Next = WalkAnim->GetFrame(FrameIndex);
	}

	void CEnemy::OnMovementState_Airborne()
	{
		LK_ASSERT(Body);
		const glm::vec2 LinearVelocity = Body->GetLinearVelocity();
		if (LinearVelocity.y > CBody::LINEAR_VELOCITY_Y_EPSILON) {
			if (const FSpriteAnimation* Anim = SpriteSheet.Find(ESpriteFrame::JumpAscend); Anim != nullptr) {
				SpriteFrame.Next = Anim->First();
			}
		} else if (LinearVelocity.y < -CBody::LINEAR_VELOCITY_Y_EPSILON) {
			if (const FSpriteAnimation* Anim = SpriteSheet.Find(ESpriteFrame::JumpDescend); Anim != nullptr) {
				SpriteFrame.Next = Anim->First();
			}
		}
	}

	void CEnemy::CheckCollisions()
	{
		if (!Body) {
			return;
		}

		constexpr int MAX_CONTACTS = 4;
		const b2BodyId BodyID = Body->GetID();
		const int Capacity = std::min(b2Body_GetContactCapacity(BodyID), MAX_CONTACTS);

		bool bGrounded = false;
		std::array<b2ContactData, MAX_CONTACTS> ContactData = { 0 };
		const int Count = b2Body_GetContactData(BodyID, ContactData.data(), Capacity);
		for (int Idx = 0; Idx < Count; Idx++) {
			const b2BodyId BodyA = b2Shape_GetBody(ContactData.at(Idx).shapeIdA);
			const float Sign = (B2_ID_EQUALS(BodyA, BodyID)) ? -1.0f : 1.0f;
			if (Sign * ContactData.at(Idx).manifold.normal.y > 0.90f) {
				bGrounded = true;
				break;
			}
		}

		if (bGrounded && bJumping) {
			bJumping = false;
			bJustLanded = true;
		}
	}

	void CEnemy::UpdateSprite()
	{
		//LK_WARN_TAG("Enemy", "Current={}  Next={}", SpriteFrame.Current, SpriteFrame.Next);
		SetSpriteTilePos(SpriteFrame.Next);
		LK_ASSERT(SpriteFrame.Current == SpriteFrame.Next);
	}

	void CEnemy::SetSpriteTilePos(const FSpriteCoord& InCoord, const bool ForceUpdate)
	{
		//LK_TRACE_TAG("Enemy", "SetSpriteTilePos: Forced={} Current={} Next={}", ForceUpdate, SpriteFrame.Current, SpriteFrame.Next); /* @todo: Remove once tested */
		if (ForceUpdate || (SpriteFrame.Current != InCoord)) {
			const bool FlipHorizontal = (Data.LookDirection == EDirection::Left);
			Sprite->SetTilePos(InCoord.X, InCoord.Y, FlipHorizontal);
			SpriteFrame.Current = InCoord;
		}
	}

	void CEnemy::ApplyMoveVelocity(const glm::vec2& InVelocity)
	{
		if (!Body) {
			return;
		}
		Body->SetLinearVelocity(InVelocity);
	}

	void CEnemy::ApplyMoveVelocityX(const float InVelocity)
	{
		if (!Body) {
			return;
		}
		const glm::vec2 CurrentVel = Body->GetLinearVelocity();
		Body->SetLinearVelocity(glm::vec2(InVelocity, CurrentVel.y));
	}

}
