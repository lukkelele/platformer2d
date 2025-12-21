#include "enemy.h"

#include "gameplaysystem.h"

#include "game/controller/patrolcontroller.h"

namespace platformer2d {

	CEnemy::CEnemy(const FEnemySpecification& InSpec, const FActorSpecification& InActorSpec, const FBodySpecification& InBodySpec)
		: CActor(InActorSpec, InBodySpec)
	{
		Data.State = EEnemyState::Idle;
		Data.LookDirection = EDirection::Right;
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
		Data.LookDirection = InDirection;
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

	bool CEnemy::Serialize(YAML::Emitter& Out, EExtendableSerializer Extendable) const
	{
		LK_WARN_TAG("Enemy", "Serializing");
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
		Out << YAML::EndMap; /* ~Actor */

		return true;
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
