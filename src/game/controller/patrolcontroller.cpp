#include "patrolcontroller.h"

#include "game/enemy.h"
#include "game/instance.h"
#include "physics/body.h"
#include "physics/ray.h"

namespace platformer2d {

	static constexpr float STUCK_THRESHOLD_SECONDS = 0.45f;
	static constexpr float STUCK_VEL_X_EPS = 0.05f;
	static constexpr float LAST_KNOWN_REACHED = 0.30f;

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
			const ETargetResponse Response = HandleTarget(Enemy, Target);
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

			if (bChasing && Enemy.GetArchetypeData().bCanJump) {
				if (const CBody* B = Enemy.GetBody()) {
					const float VelX = B->GetLinearVelocity().x;
					const bool MoveRequested = (Response == ETargetResponse::MoveLeft) || (Response == ETargetResponse::MoveRight);
					if (MoveRequested && (std::abs(VelX) < STUCK_VEL_X_EPS)) {
						StuckTimer += DeltaTime;
						if (StuckTimer >= STUCK_THRESHOLD_SECONDS) {
							LK_DEBUG_TAG("PatrolController", "[{}] Stuck, jumping", Enemy.GetName()); /* @todo: Change to trace later on */
							Enemy.Jump();
							StuckTimer = 0.0f;
						}
					} else {
						StuckTimer = 0.0f;
					}
				}
			} else {
				StuckTimer = 0.0f;
			}

			if ((Response == ETargetResponse::None) && !bPlayerLOS) {
				Patrol(Enemy);
			}
		} else {
			Patrol(Enemy);
		}
	}

	void CPatrolController::SetTarget(std::shared_ptr<CActor> InTarget)
	{
		LK_DEBUG_TAG("PatrolController", "Targeting: {}", (InTarget ? InTarget->GetName() : "None"));
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

	CPatrolController::ETargetResponse CPatrolController::HandleTarget(CEnemy& Enemy, const std::shared_ptr<CActor>& Target)
	{
		if (!Target) {
			return ETargetResponse::None;
		}

		const glm::vec3 CurrentPos3 = Enemy.GetPosition();
		const glm::vec2 Pos = glm::vec2(CurrentPos3.x, CurrentPos3.y);
		const glm::vec3 TargetPos3 = Target->GetPosition();
		const glm::vec2 TargetPos = glm::vec2(TargetPos3.x, TargetPos3.y);

		const glm::vec2 Delta = TargetPos - Pos;
		const float DistSq = glm::dot(Delta, Delta);

		const FEnemyArchetype& Archetype = Enemy.GetArchetypeData();
		const float DetectRadiusSq = Archetype.DetectRadius * Archetype.DetectRadius;
		const float StopRadiusSq = Archetype.StopRadius * Archetype.StopRadius;
		const float GiveUpRadiusSq = Archetype.GiveUpRadius * Archetype.GiveUpRadius;

		bPlayerLOS = Physics::HasLineOfSight(Pos, TargetPos, Target.get());
		// LK_DEBUG_TAG("PatrolController", "PatrolDirection={}  LOS={}", Enum::ToString(PatrolDirection), bPlayerLOS);

		if (!bChasing) {
			if ((DistSq <= DetectRadiusSq) && bPlayerLOS) {
				bChasing = true;
				bHasLastKnownPos = false;
			}
		} else {
			if (DistSq > GiveUpRadiusSq) {
				bChasing = false;
			} else if (!bPlayerLOS) {
				bChasing = false;
				LastKnownPos = TargetPos;
				bHasLastKnownPos = true;
			}
		}

		if (bChasing) {
			LastKnownPos = TargetPos;
			bHasLastKnownPos = true;

			if (DistSq <= StopRadiusSq) {
				return ETargetResponse::StopMovement;
			}

			constexpr float THRESHOLD_X = 0.10f;
			const float DeltaX = TargetPos.x - Pos.x;
			if (std::abs(DeltaX) <= THRESHOLD_X) {
				return ETargetResponse::StopMovement;
			}

			return (DeltaX < 0.0f) ? ETargetResponse::MoveLeft : ETargetResponse::MoveRight;
		}

		if (bHasLastKnownPos) {
			const float DeltaX = LastKnownPos.x - Pos.x;
			if (std::abs(DeltaX) <= LAST_KNOWN_REACHED) {
				bHasLastKnownPos = false;
				return ETargetResponse::StopMovement;
			}

			return (DeltaX < 0.0f) ? ETargetResponse::MoveLeft : ETargetResponse::MoveRight;
		}

		return ETargetResponse::None;
	}

	void CPatrolController::Patrol(CEnemy& Enemy)
	{
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
