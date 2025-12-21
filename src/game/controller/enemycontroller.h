#pragma once

#include "game/controller.h"

namespace platformer2d {

	class CEnemy;

	enum class EEnemyState
	{
		Idle,
		Patrolling,
		COUNT
	};

	struct FEnemyData
	{
		LUUID ID = 0;
		EEnemyState State = EEnemyState::Idle;
		EDirection LookDirection = EDirection::Right;
	};

	class IEnemyController : public IController
	{
	public:
		virtual ~IEnemyController() = default;

		virtual void OnPossess(CEnemy& Enemy) {}
		virtual void Tick(CEnemy& Enemy, float DeltaTime) = 0;
	};

	namespace Enum {
		inline constexpr const char* ToString(const EEnemyState State)
		{
			const char* S = "";
		#define _(EnumValue) case EEnemyState::EnumValue: S = #EnumValue; break
			switch (State) {
				_(Idle);
				_(Patrolling);
				_(COUNT);
				default:
					S = nullptr;
					break;
			}
		#undef _
			return S;
		}
	}

}
