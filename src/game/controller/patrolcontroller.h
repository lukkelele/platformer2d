#pragma once

#include "enemycontroller.h"

namespace platformer2d {

	class CPatrolController : public IEnemyController
	{
	public:
		CPatrolController(float InPatrolHalfDistance, float InStartDelaySeconds = 0.0f);
		~CPatrolController() = default;

		virtual void OnPossess(CEnemy& Enemy) override;
		virtual void Tick(CEnemy& Enemy, float DeltaTime) override;

		virtual EControllerType GetControllerType() const override { return EControllerType::Patrol; }

		void SetPatrolDirection(EDirection InDirection);
		EDirection GetPatrolDirection() const { return PatrolDirection; }

		float GetHalfDistance() const { return PatrolHalfDistance; }
		float GetStartDelayInSeconds() const { return StartDelaySeconds; }

	private:
		float StartX = 0.0f;
		float PatrolHalfDistance = 2.0f;

		float StartDelaySeconds = 0.0f;
		float ElapsedSeconds = 0.0f;

		EDirection PatrolDirection = EDirection::Right;
	};

}
