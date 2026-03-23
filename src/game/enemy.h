#pragma once

#include "scene/actor.h"
#include "controller/enemycontroller.h"

namespace platformer2d {

	struct FEnemySpecification
	{
		EControllerType ControllerType = EControllerType::None;
		glm::vec2 SpawnPoint{};
	};

	class CEnemy : public CActor
	{
	public:
		CEnemy(const FEnemySpecification& InSpec, const FActorSpecification& InActorSpec, const FBodySpecification& InBodySpec);
		virtual ~CEnemy();

		virtual void Tick(float DeltaTime) override;
		virtual EActorType GetActorType() const override { return EActorType::Enemy; }

		void SetController(std::unique_ptr<IEnemyController> InController);
		IEnemyController* GetController() const { return Controller.get(); }
		bool HasController() const { return (Controller != nullptr); }

		inline const FEnemyData& GetData() const { return Data; }
		inline FEnemyData& GetData() { return Data; }

		EEnemyState GetState() const { return Data.State; }
		void SetState(EEnemyState InState);

		void SetLookDirection(EDirection InDirection);
		void SetMoveSpeed(float InSpeed);
		float GetMoveSpeed() const { return MoveSpeed; }

		void MoveLeft();
		void MoveRight();
		void StopMovement();

		enum class EReviveVariant
		{
			AtSpawn,
			AtLastLocation
		};

		void Kill();
		void Revive(EReviveVariant Variant = EReviveVariant::AtSpawn);
		bool IsDead() const;

		void SetSpawnPoint(const glm::vec2& InPoint);
		const glm::vec2& GetSpawnPoint() const { return Data.SpawnPoint; }

		virtual bool Serialize(YAML::Emitter& Out, EExtendableSerializer Extendable = EExtendableSerializer::No) const override;

	private:
		void ApplyMoveVelocity(const glm::vec2& InVelocity);
		void ApplyMoveVelocityX(float InVelocity);

	private:
		FEnemyData Data{};
		std::unique_ptr<IEnemyController> Controller = nullptr;

		float MoveSpeed = 1.20f;
	};

	namespace Enum {
		inline const char* ToString(const CEnemy::EReviveVariant Variant)
		{
			const char* S = "";
#define _(EnumValue)                                              \
	case CEnemy::EReviveVariant::EnumValue: S = #EnumValue; break
			switch (Variant) {
				_(AtSpawn);
				_(AtLastLocation);
				default:
					LK_THROW_ENUM_ERR(Variant);
					break;
			}
#undef _
			return S;
		}
	}

}
