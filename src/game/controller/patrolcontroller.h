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

		[[nodiscard]] EControllerType GetControllerType() const override { return EControllerType::Patrol; }

		void SetTarget(std::shared_ptr<CActor> InTarget);
		[[nodiscard]] bool HasTarget() const { return !TargetRef.expired(); }

		void SetPatrolDirection(EDirection InDirection);
		[[nodiscard]] EDirection GetPatrolDirection() const { return PatrolDirection; }

		[[nodiscard]] bool IsChasing() const { return bChasing; }
		[[nodiscard]] float GetHalfDistance() const { return PatrolHalfDistance; }
		[[nodiscard]] float GetStartDelayInSeconds() const { return StartDelaySeconds; }
		void SetDetectRadius(float InRadius);
		[[nodiscard]] float GetDetectRadius() const { return DetectRadius; }
		void SetStopRadius(float InRadius);
		[[nodiscard]] float GetStopRadius() const { return StopRadius; }

	private:
		enum class ETargetResponse
		{
			None,
			StopMovement,
			MoveLeft,
			MoveRight,
			COUNT
		};
		LK_ENUM(ETargetResponse);

		[[nodiscard]] ETargetResponse HandleTarget(CEnemy& Enemy, const std::shared_ptr<CActor>& Target);
		void Patrol(CEnemy& Enemy);

		[[nodiscard]] bool HasGroundAhead(CEnemy& Enemy, EDirection Dir) const;

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
		bool bPlayerLOS = false;

		bool bHasLastKnownPos = false;
		glm::vec2 LastKnownPos = {0.0f, 0.0f};
		float StuckTimer = 0.0f;
	};
}

