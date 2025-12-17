#pragma once

#include <box2d/box2d.h>

#include "core/core.h"
#include "core/math/math.h"
#include "renderer/color.h"
#include "scene/actor.h"
#include "item.h"
#include "weapontype.h"

namespace platformer2d {

	class IWeapon : public CActor, public IItem
	{
	public:
		virtual ~IWeapon() = default;

		virtual void Tick(float DeltaTime) = 0;
		virtual void Render() = 0;
		virtual EWeaponType GetWeaponType() const = 0;

		virtual EItemType GetItemType() const override { return EItemType::Weapon; }
	};

}
