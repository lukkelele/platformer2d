#pragma once

#include <cstdint>
#include "core/enum.h"

namespace platformer2d {

	enum class EMouseButton : std::uint16_t
	{
		Button0, /* Left   */
		Button1, /* Right  */
		Button2, /* Middle */
		Button3,
		Button4,
		Button5,
		None,
		COUNT,

		/* Keep aliases below the rest. */
		Left = Button0,
		Right = Button1,
		Middle = Button2,
	};
	LK_ENUM(EMouseButton);

	enum class EMouseButtonState : std::int8_t
	{
		None = -1,
		Pressed,
		Held,
		Released,
		COUNT
	};
	LK_ENUM_RANGE(EMouseButtonState, EMouseButtonState::None, EMouseButtonState::COUNT);

	enum class ECursorMode
	{
		Normal,
		Hidden,
		Locked,
		COUNT
	};
	LK_ENUM(ECursorMode);

	enum class EMouseScrollDirection : std::uint8_t
	{
		Up,
		Down,
		COUNT
	};
	LK_ENUM(EMouseScrollDirection);

}

