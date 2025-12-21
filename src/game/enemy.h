#pragma once

#include "scene/actor.h"
#include "controller/enemycontroller.h"

namespace platformer2d {

	struct FEnemySpecification
	{
		EControllerType ControllerType = EControllerType::None;
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

		virtual bool Serialize(YAML::Emitter& Out, EExtendableSerializer Extendable = EExtendableSerializer::No) const override;

	private:
		void ApplyMoveVelocity(const glm::vec2& InVelocity);
		void ApplyMoveVelocityX(float InVelocity);

	private:
		FEnemyData Data{};
		std::unique_ptr<IEnemyController> Controller = nullptr;

		float MoveSpeed = 1.20f;
	};

}
