#include "patrolcontroller.h"

#include "game/enemy.h"

namespace platformer2d {

	CPatrolController::CPatrolController(const float InPatrolHalfDistance, const float InStartDelaySeconds)
		: PatrolHalfDistance(InPatrolHalfDistance)
		, StartDelaySeconds(InStartDelaySeconds)
	{
		LK_DEBUG_TAG("PatrolController", "Created  HalfDistance={} StartDelaySeconds={}", PatrolHalfDistance, StartDelaySeconds);
	}

	void CPatrolController::OnPossess(CEnemy& Enemy)
	{
		LK_WARN_TAG("PatrolController", "OnPossess -> {}", Enemy.GetName());
		StartX = Enemy.GetPosition().x;
		Enemy.GetData().State = EEnemyState::Patrolling;
		PatrolDirection = Enemy.GetData().LookDirection;
		ElapsedSeconds = 0.0f;
	}

	void CPatrolController::Tick(CEnemy& Enemy, const float DeltaTime)
	{
		if (Enemy.GetData().State != EEnemyState::Patrolling) {
			Enemy.StopMovement();
			return;
		}

		if ((StartDelaySeconds > 0.0f) && (ElapsedSeconds < StartDelaySeconds)) {
			ElapsedSeconds += DeltaTime;
			Enemy.StopMovement();
			return;
		}

		const float X = Enemy.GetPosition().x;
		const float MinX = StartX - PatrolHalfDistance;
		const float MaxX = StartX + PatrolHalfDistance;

		if ((PatrolDirection == EDirection::Right) && (X >= MaxX)) {
			PatrolDirection = EDirection::Left;
		}
		if ((PatrolDirection == EDirection::Left) && (X <= MinX)) {
			PatrolDirection = EDirection::Right;
		}

		if (PatrolDirection == EDirection::Left) {
			Enemy.MoveLeft();
		} else if (PatrolDirection == EDirection::Right) {
			Enemy.MoveRight();
		}
	}

	void CPatrolController::SetPatrolDirection(const EDirection InDirection)
	{
		PatrolDirection = InDirection;
	}

}
