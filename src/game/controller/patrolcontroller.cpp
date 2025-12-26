#include "patrolcontroller.h"

#include "game/enemy.h"
#include "game/instance.h"

namespace platformer2d {

	CPatrolController::CPatrolController(const float InPatrolHalfDistance, const float InStartDelaySeconds)
		: PatrolHalfDistance(InPatrolHalfDistance)
		, StartDelaySeconds(InStartDelaySeconds)
	{
		LK_DEBUG_TAG("PatrolController", "Created  HalfDistance={} StartDelaySeconds={}", PatrolHalfDistance, StartDelaySeconds);
	}

	void CPatrolController::OnPossess(CEnemy& Enemy)
	{
		LK_DEBUG_TAG("PatrolController", "OnPossess -> {}", Enemy.GetName());
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

		if (std::shared_ptr<CActor> Target = TargetRef.lock(); Target != nullptr) {
			const ETargetResponse Response = HandleTarget(Enemy.GetPosition(), Target);
			switch (Response) {
				case ETargetResponse::None:
					break;
				case ETargetResponse::MoveLeft:
					Enemy.MoveLeft();
					break;
				case ETargetResponse::MoveRight:
					Enemy.MoveRight();
					break;
				case ETargetResponse::StopMovement:
					Enemy.StopMovement();
					break;
			}
		} else {
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
	}

	void CPatrolController::SetTarget(std::shared_ptr<CActor> InTarget)
	{
		LK_DEBUG_TAG("PatrolController", "Targeting: {}", (InTarget ? InTarget->GetName() : "NULL"));
		TargetRef = InTarget;
	}

	void CPatrolController::SetPatrolDirection(const EDirection InDirection)
	{
		PatrolDirection = InDirection;
	}

	void CPatrolController::SetDetectRadius(const float InRadius)
	{
		DetectRadius = InRadius;
	}

	void CPatrolController::SetStopRadius(const float InRadius)
	{
		StopRadius = InRadius;
	}

	CPatrolController::ETargetResponse CPatrolController::HandleTarget(const glm::vec3& CurrentPos, const std::shared_ptr<CActor>& Target)
	{
		if (!Target) {
			return ETargetResponse::None;
		}

		const glm::vec2 Pos = glm::vec2(CurrentPos.x, CurrentPos.y);
		const glm::vec3 TargetPos3 = Target->GetPosition();
		const glm::vec2 TargetPos = glm::vec2(TargetPos3.x, TargetPos3.y);

		const glm::vec2 Delta = TargetPos - Pos;
		const float DistSq = glm::dot(Delta, Delta);

		const float DetectRadiusSq = DetectRadius * DetectRadius;
		const float StopRadiusSq = StopRadius * StopRadius;
		const float GiveUpRadiusSq = GiveUpRadius * GiveUpRadius;

		if (!bChasing) {
			if (DistSq <= DetectRadiusSq) {
				bChasing = true;
			}
		} else {
			if (DistSq > GiveUpRadiusSq) {
				bChasing = false;
			}
		}

		if (bChasing) {
			if (DistSq <= StopRadiusSq) {
				return ETargetResponse::StopMovement;
			}

			static constexpr float THRESHOLD_X = 0.10f;
			const float DeltaX = TargetPos.x - CurrentPos.x;

			if (std::abs(DeltaX) <= THRESHOLD_X) {
				return ETargetResponse::StopMovement;
			}

			if (DeltaX < 0.0f) {
				return ETargetResponse::MoveLeft;
			} else {
				return ETargetResponse::MoveRight;
			}
		}

		return ETargetResponse::None;
	}

}
