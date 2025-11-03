#pragma once

#include "lk_config.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <span>

#include "assert.h"
#include "log.h"
#include "macros.h"

#define LK_MARK_NOT_IMPLEMENTED() \
	LK_VERIFY(false, "Not implemented: {}::{}", __FILE__, __LINE__)

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

}
