#pragma once

#include <cstdint>

namespace platformer2d {

	enum class EMouseButton : uint16_t
	{
		Button0, /* Left   */
		Button1, /* Right  */
		Button2, /* Middle */
		Button3,
		Button4,
		Button5,
		None,

		/* Keep aliases below the rest. */
		Left   = Button0,
		Right  = Button1,
		Middle = Button2,
	};

	enum class EMouseButtonState
	{
		None = -1,
		Pressed,
		Held,
		Released
	};

	enum class ECursorMode
	{
		Normal = 0,
		Hidden = 1,
		Locked = 2
	};

	enum class EMouseScrollDirection
	{
		Up,
		Down
	};

	namespace Enum
	{
		inline const char* ToString(const EMouseButton MouseButton)
		{
			switch (MouseButton)
			{
				case EMouseButton::Button0: return "Button0";
				case EMouseButton::Button1: return "Button1";
				case EMouseButton::Button2: return "Button2";
				case EMouseButton::Button3: return "Button3";
				case EMouseButton::Button4: return "Button4";
				case EMouseButton::Button5: return "Button5";
				case EMouseButton::None:    return "None";
			}
			LK_VERIFY(false);
			return nullptr;
		}

		inline const char* ToString(const ECursorMode CursorMode)
		{
			switch (CursorMode)
			{
				case ECursorMode::Normal: return "Normal";
				case ECursorMode::Hidden: return "Hidden";
				case ECursorMode::Locked: return "Locked";
			}
			LK_VERIFY(false);
			return nullptr;
		}

		inline const char* ToString(const EMouseScrollDirection ScrollDirection)
		{
			switch (ScrollDirection)
			{
				case EMouseScrollDirection::Up:   return "Up";
				case EMouseScrollDirection::Down: return "Down";
			}
			LK_VERIFY(false);
			return nullptr;
		}
	}

}