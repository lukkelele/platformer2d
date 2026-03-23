#pragma once

#include "enemycontroller.h"

namespace platformer2d {

	class CActor;

	class CPatrolController : public IEnemyController
	{
	public:
		CPatrolController(float InPatrolHalfDistance, float InStartDelaySeconds = 0.0f);
		~CPatrolController() = default;

		void OnPossess(CEnemy& Enemy) override;
		void Tick(CEnemy& Enemy, float DeltaTime) override;

		EControllerType GetControllerType() const override { return EControllerType::Patrol; }

		void SetTarget(std::shared_ptr<CActor> InTarget);
		bool HasTarget() const { return !TargetRef.expired(); }

		void SetPatrolDirection(EDirection InDirection);
		EDirection GetPatrolDirection() const { return PatrolDirection; }

		bool IsChasing() const { return bChasing; }
		float GetHalfDistance() const { return PatrolHalfDistance; }
		float GetStartDelayInSeconds() const { return StartDelaySeconds; }
		void SetDetectRadius(float InRadius);
		float GetDetectRadius() const { return DetectRadius; }
		void SetStopRadius(float InRadius);
		float GetStopRadius() const { return StopRadius; }

	private:
		enum class ETargetResponse
		{
			None,
			StopMovement,
			MoveLeft,
			MoveRight
		};
		ETargetResponse HandleTarget(const glm::vec3& CurrentPos, const std::shared_ptr<CActor>& Target);

	private:
		float StartX = 0.0f;
		float PatrolHalfDistance = 2.0f;
		std::weak_ptr<CActor> TargetRef;
		EDirection PatrolDirection = EDirection::Right;

		float StartDelaySeconds = 0.0f;
		float ElapsedSeconds = 0.0f;

		bool bChasing = false;
		float DetectRadius = 4.0f;
		float StopRadius = 0.60f;
		float GiveUpRadius = 6.0f;
	};

}
