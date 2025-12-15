#pragma once

#include <box2d/box2d.h>

#include "core/core.h"
#include "core/math/math.h"
#include "renderer/color.h"
#include "scene/actor.h"
#include "item.h"

namespace platformer2d {

	enum class EWeaponType
	{
		Rifle,
		COUNT
	};

	class IWeapon : public CActor, public IItem
	{
	public:
		virtual ~IWeapon() = default;

		virtual void Tick() = 0;
		virtual void Render() = 0;
		virtual EWeaponType GetWeaponType() const = 0;

		virtual EItemType GetItemType() const override { return EItemType::Weapon; }
	};

	namespace Enum {
		inline const char* ToString(const EWeaponType Type)
		{
			const char* S = "";
		#define _(EnumValue) case EWeaponType::EnumValue: S = #EnumValue; break
			switch (Type) {
				_(Rifle);
				_(COUNT);
				default:
					LK_THROW_ENUM_ERR(Type);
					break;
			}
		#undef _
			return S;
		}
	}

}
