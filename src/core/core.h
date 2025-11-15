#pragma once

#include "lk_config.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <source_location>
#include <span>

#include "assert.h"
#include "log.h"
#include "macros.h"
#include "uuid.h"

#define LK_MARK_NOT_IMPLEMENTED() \
	LK_VERIFY(false, "Not implemented: {}::{}", __FILE__, __LINE__)

#define LK_THROW_ENUM_ERR(EnumValue) \
	LK_VERIFY(false, "{} failed with value: {}", std::source_location::current().function_name(), ::platformer2d::Enum::AsUnderlying(EnumValue));

namespace platformer2d {

	using LRendererID = uint32_t;

	namespace Core
	{
		int ParseSvgPath(std::string_view, const glm::vec2& Offset,
						 std::span<glm::vec2> Points, float Scale, bool ReverseOrder);

		struct FGlobal
		{
			bool bShouldShutdown = false;
		};
		extern FGlobal Global;
	}

	namespace Enum
	{
		template<typename TEnum>
		constexpr std::underlying_type_t<TEnum> AsUnderlying(const TEnum E)
		{
			return static_cast<std::underlying_type_t<TEnum>>(E);
		}
	}

}
