#pragma once

#include <cstdint>

#include "core/core.h"
#include "core/enum.h"

namespace platformer2d {

	/**
	 * @enum EPadButton
	 * @brief Gamepad buttons using GLFW standard mapping.
	 */
	enum class EPadButton : std::uint8_t
	{
		A			= 0,
		B			= 1,
		X			= 2,
		Y			= 3,
		LeftBumper	= 4,
		RightBumper	= 5,
		Back		= 6,
		Start		= 7,
		Guide		= 8,
		LeftThumb	= 9,
		RightThumb	= 10,
		DPadUp		= 11,
		DPadRight	= 12,
		DPadDown	= 13,
		DPadLeft	= 14,
		None		= 15,
		COUNT		= 16,

		/* Keep aliases below the rest (PlayStation glyphs). */
		Cross		= A,
		Circle		= B,
		Square		= X,
		Triangle	= Y,
	};
	LK_ENUM(EPadButton);

	enum class EPadButtonState : std::int8_t
	{
		None = -1,
		Pressed,
		Held,
		Released,
		COUNT
	};
	LK_ENUM_RANGE(EPadButtonState, EPadButtonState::None, EPadButtonState::COUNT);

	/**
	 * @enum EPadAxis
	 * @brief Gamepad analog axes using GLFW standard mapping.
	 * Sticks report [-1, 1]. Triggers report [-1, 1] from GLFW but are remapped to [0, 1] by CInputSystem.
	 */
	enum class EPadAxis : std::uint8_t
	{
		LeftStickX	 = 0,
		LeftStickY	 = 1,
		RightStickX	 = 2,
		RightStickY	 = 3,
		LeftTrigger	 = 4,
		RightTrigger = 5,
		None		 = 6,
		COUNT		 = 7
	};
	LK_ENUM(EPadAxis);

	struct FPadButtonData
	{
		EPadButton Button = EPadButton::None;
		EPadButtonState State = EPadButtonState::None;
		EPadButtonState OldState = EPadButtonState::None;
		std::int32_t PadId = -1;
	};

	struct FPadAxisData
	{
		EPadAxis Axis = EPadAxis::None;
		float Value = 0.0f;
		float OldValue = 0.0f;
		std::int32_t PadId = -1;
	};

}

template<>
struct lklog::fmt::formatter<platformer2d::EPadButton>
{
	template<typename ParseContext>
	constexpr auto parse(ParseContext& Context)
	{
		return Context.begin();
	}

	template<typename FormatContext>
	auto format(const platformer2d::EPadButton Button, FormatContext& Context) const
	{
		return lklog::fmt::format_to(Context.out(), "{}", platformer2d::Enum::ToString(Button));
	}
};

template<>
struct lklog::fmt::formatter<platformer2d::EPadAxis>
{
	template<typename ParseContext>
	constexpr auto parse(ParseContext& Context)
	{
		return Context.begin();
	}

	template<typename FormatContext>
	auto format(const platformer2d::EPadAxis Axis, FormatContext& Context) const
	{
		return lklog::fmt::format_to(Context.out(), "{}", platformer2d::Enum::ToString(Axis));
	}
};
