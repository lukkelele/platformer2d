#pragma once

#include "core/enum.h"
#include "game/controller.h"

namespace platformer2d {

	class CEnemy;

	enum class EEnemyState
	{
		Idle,
		Patrolling,
		COUNT
	};
	LK_ENUM(EEnemyState);

	struct FEnemyData
	{
		LUUID ID = 0;
		EEnemyState State = EEnemyState::Idle;
		EDirection LookDirection = EDirection::Right;
		glm::vec2 SpawnPoint{};
	};

	class IEnemyController : public IController
	{
	public:
		virtual ~IEnemyController() = default;

		virtual void OnPossess(CEnemy& Enemy) = 0;
		virtual void Tick(CEnemy& Enemy, float DeltaTime) = 0;
	};

}

