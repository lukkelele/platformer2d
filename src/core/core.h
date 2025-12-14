#pragma once

#include "lk_config.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
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
	LK_VERIFY(false, "{} failed with value: {}", std::source_location::current().function_name(), std::to_underlying(EnumValue));

namespace platformer2d {

	using LRendererID = uint32_t;

	using namespace std::chrono_literals;

	namespace Core {
		static const std::filesystem::path ProjectDir = std::filesystem::weakly_canonical(PROJECT_DIR);

		struct FGlobal
		{
			bool bShouldShutdown = false;
		};
		extern FGlobal Global;

		/**
		 * @brief Get relative path from the project directory.
		 */
		std::filesystem::path GetRelativeFromProject(const std::filesystem::path& Input);

		int ParseSvgPath(std::string_view, const glm::vec2& Offset,
						 std::span<glm::vec2> Points, float Scale, bool ReverseOrder);

		/**
		 * @brief Run asynchronously.
		 */
		template<typename F, typename... TArgs>
		static void RunDetachedAfter(const std::chrono::steady_clock::duration& Delay, F&& Func, TArgs&&... Args)
		{
			std::thread(
				[Delay, Func = std::forward<F>(Func), ...Args = std::forward<TArgs>(Args)]()
				{
					std::this_thread::sleep_for(Delay);
					Func(std::forward<TArgs>(Args)...);
				}
			).detach();
		}
	}

	enum class EDirection
	{
		Up,
		Down,
		Left,
		Right
	};

	namespace Enum {
		inline const char* ToString(const EDirection Direction)
		{
			const char* S = "";
		#define _(EnumValue) case EDirection::EnumValue: S = #EnumValue; break
			switch (Direction) {
				_(Up);
				_(Down);
				_(Left);
				_(Right);
				default:
					LK_THROW_ENUM_ERR(Direction);
					break;
			}
		#undef _
			return S;
		}
	}

}
