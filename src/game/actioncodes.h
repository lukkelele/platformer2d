#pragma once

#include "core/core.h"
#include "core/enum.h"

namespace platformer2d {

	enum class EAction : std::uint16_t
	{
		None = 0,
		Jump,
		Fire,
		Interact,
		Pause,
		MoveLeft,
		MoveRight,
		MoveUp,
		MoveDown,
		COUNT
	};
	LK_ENUM(EAction);

	enum class EAxis : std::uint8_t
	{
		None = 0,
		MoveX,
		MoveY,
		AimX,
		AimY,
		COUNT
	};
	LK_ENUM(EAxis);

	enum class EActionState : std::int8_t
	{
		None = -1,
		Pressed,
		Held,
		Released,
		COUNT
	};
	LK_ENUM_RANGE(EActionState, EActionState::None, EActionState::COUNT);

	struct FActionData
	{
		EAction Action = EAction::None;
		EActionState State = EActionState::None;
		EActionState OldState = EActionState::None;
	};

	struct FAxisData
	{
		EAxis Axis = EAxis::None;
		float Value = 0.0f;
		float OldValue = 0.0f;
	};
}

template<>
struct lklog::fmt::formatter<platformer2d::EAction>
{
	template<typename ParseContext>
	constexpr auto parse(ParseContext& Context)
	{
		return Context.begin();
	}

	template<typename FormatContext>
	auto format(const platformer2d::EAction Action, FormatContext& Context) const
	{
		return lklog::fmt::format_to(Context.out(), "{}", platformer2d::Enum::ToString(Action));
	}
};

template<>
struct lklog::fmt::formatter<platformer2d::EAxis>
{
	template<typename ParseContext>
	constexpr auto parse(ParseContext& Context)
	{
		return Context.begin();
	}

	template<typename FormatContext>
	auto format(const platformer2d::EAxis Axis, FormatContext& Context) const
	{
		return lklog::fmt::format_to(Context.out(), "{}", platformer2d::Enum::ToString(Axis));
	}
};

