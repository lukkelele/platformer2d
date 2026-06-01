#pragma once

#include "core/core.h"

namespace platformer2d {

	enum ECollisionCategory : std::uint64_t
	{
		ECollisionCategory_World = LK_BIT(0), /* Box2d default. */
		ECollisionCategory_Player = LK_BIT(1),
		ECollisionCategory_Enemy = LK_BIT(2),
		ECollisionCategory_Projectile = LK_BIT(3),
	};

	namespace Physics {
		/**
		 * @brief Ground probe mask.
		 */
		inline constexpr std::uint64_t COLLISION_QUERY_GROUND =
			~(ECollisionCategory_Player | ECollisionCategory_Enemy | ECollisionCategory_Projectile);

		/**
		 * @brief Line of sight mask.
		 */
		inline constexpr std::uint64_t COLLISION_QUERY_SIGHT =
			~(ECollisionCategory_Enemy | ECollisionCategory_Projectile);
	}

}
