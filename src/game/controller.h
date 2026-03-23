#pragma once

#include "core/core.h"

namespace platformer2d {

	enum class EControllerType
	{
		None,
		Patrol,
		COUNT
	};

	class IController
	{
	public:
		virtual ~IController() = default;

		virtual EControllerType GetControllerType() const = 0;
	};

	namespace Enum {
		inline const char* ToString(const EControllerType Type)
		{
			const char* S = "";
#define _(EnumValue)                                       \
	case EControllerType::EnumValue: S = #EnumValue; break
			switch (Type) {
				_(None);
				_(Patrol);
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
