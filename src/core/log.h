#pragma once

#include <lklog/lklog.h>

#include "log_formatters.h"

namespace platformer2d {
	template<typename... TArgs>
	inline std::string Format(lklog::fmt::format_string<TArgs...> Fmt, TArgs&&... Args)
	{
		return lklog::format(Fmt, std::forward<TArgs>(Args)...);
	}
}

#define LK_TRACE(...) LKLOG_TRACE(__VA_ARGS__)
#define LK_DEBUG(...) LKLOG_DEBUG(__VA_ARGS__)
#define LK_INFO(...)  LKLOG_INFO(__VA_ARGS__)
#define LK_WARN(...)  LKLOG_WARN(__VA_ARGS__)
#define LK_ERROR(...) LKLOG_ERROR(__VA_ARGS__)
#define LK_FATAL(...) LKLOG_FATAL(__VA_ARGS__)

#define LK_TRACE_TAG(_tag, ...) LKLOG_TRACE_TAG(_tag, __VA_ARGS__)
#define LK_DEBUG_TAG(_tag, ...) LKLOG_DEBUG_TAG(_tag, __VA_ARGS__)
#define LK_INFO_TAG(_tag, ...)  LKLOG_INFO_TAG(_tag, __VA_ARGS__)
#define LK_WARN_TAG(_tag, ...)  LKLOG_WARN_TAG(_tag, __VA_ARGS__)
#define LK_ERROR_TAG(_tag, ...) LKLOG_ERROR_TAG(_tag, __VA_ARGS__)
#define LK_FATAL_TAG(_tag, ...) LKLOG_FATAL_TAG(_tag, __VA_ARGS__)

