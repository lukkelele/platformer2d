#pragma once

#include "controller/enemycontroller.h"
#include "game/enemyarchetype.h"
#include "game/spritereader.h"
#include "renderer/sprite.h"
#include "scene/actor.h"

namespace platformer2d {

	struct FEnemySpecification
	{
		EControllerType ControllerType = EControllerType::None;
		EEnemyArchetype Archetype = EEnemyArchetype::Grunt;
		glm::vec2 SpawnPoint{};
	};

	class CEnemy : public CActor
	{
	public:
		CEnemy(const FEnemySpecification& InSpec, const FActorSpecification& InActorSpec, const FBodySpecification& InBodySpec);
		CEnemy(CEnemy&&) = delete;
		CEnemy(const CEnemy&) = delete;
		virtual ~CEnemy();

		void Tick(float DeltaTime) override;
		void OnDeath(EDeathReason Reason) override;
		[[nodiscard]] EActorType GetActorType() const override { return EActorType::Enemy; }

		void SetController(std::unique_ptr<IEnemyController> InController);
		[[nodiscard]] IEnemyController* GetController() const { return Controller.get(); }
		[[nodiscard]] bool HasController() const { return (Controller != nullptr); }

		[[nodiscard]] const FEnemyData& GetData() const { return Data; }
		[[nodiscard]] FEnemyData& GetData() { return Data; }

		[[nodiscard]] EEnemyState GetState() const { return Data.State; }
		void SetState(EEnemyState InState);

		void SetLookDirection(EDirection InDirection);
		void SetMoveSpeed(float InSpeed);
		[[nodiscard]] float GetMoveSpeed() const { return MoveSpeed; }

		void MoveLeft();
		void MoveRight();
		void StopMovement();
		void Jump();

		enum class EReviveVariant
		{
			AtSpawn,
			AtLastLocation,
			COUNT
		};
		LK_ENUM(EReviveVariant);

		void Kill();
		void Revive(EReviveVariant Variant = EReviveVariant::AtSpawn);
		[[nodiscard]] bool IsDead() const;

		void SetSpawnPoint(const glm::vec2& InPoint);
		[[nodiscard]] const glm::vec2& GetSpawnPoint() const { return Data.SpawnPoint; }

		[[nodiscard]] EEnemyArchetype GetArchetype() const { return ArchetypeKind; }
		[[nodiscard]] const FEnemyArchetype& GetArchetypeData() const { return GetEnemyArchetype(ArchetypeKind); }

		[[nodiscard]] const CSprite* GetSprite() const override { return Sprite.get(); }

		bool Serialize(YAML::Emitter& Out, EExtendableSerializer Extendable = EExtendableSerializer::No) const override;

	private:
		void SetMovementState(EMovementState State);
		void UpdateMovementState();
		void OnMovementState_Idle();
		void OnMovementState_Running();
		void OnMovementState_Airborne();

		void CheckCollisions();
		void UpdateSprite();
		void SetSpriteTilePos(const FSpriteCoord& InCoord, bool ForceUpdate = false);

		void ApplyMoveVelocity(const glm::vec2& InVelocity);
		void ApplyMoveVelocityX(float InVelocity);

	private:
		FEnemyData Data{};
		std::unique_ptr<IEnemyController> Controller;
		EEnemyArchetype ArchetypeKind = EEnemyArchetype::Grunt;

		float MoveSpeed = 1.20f;
		EMovementState MovementState = EMovementState::Idle;
		bool bJumping = false;
		bool bJustLanded = false;

		std::unique_ptr<CSprite> Sprite;
		const FSpriteSheet* SpriteSheet = nullptr;
		FSpriteFrame SpriteFrame;
		bool bShouldUpdateSprite = false;
	};
}

