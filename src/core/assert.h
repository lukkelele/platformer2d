#pragma once

#include <cassert>

#include "platform.h"
#include "log.h"

/**
 * Assert.
 */
#ifdef LK_ENABLE_ASSERT
#	ifdef __LK_VA_OPT
#		define __LK_ASSERT_MESSAGE(...) ::platformer2d::CLog::PrintAssertMessage(::platformer2d::ELoggerType::Core, "Assert failed" __VA_OPT__(, ) __VA_ARGS__)
#	else
#		define __LK_ASSERT_MESSAGE(...) ::platformer2d::CLog::PrintAssertMessage(::platformer2d::ELoggerType::Core, "Assert failed", ##__VA_ARGS__)
#	endif
#define LK_ASSERT(Condition, ...)                        \
	{                                                    \
		if (!(Condition))                                \
		{                                                \
			if constexpr (sizeof(#__VA_ARGS__) > 1)      \
			{                                            \
				__LK_ASSERT_MESSAGE(__VA_ARGS__);        \
			}                                            \
			else                                         \
			{                                            \
				__LK_ASSERT_MESSAGE("{}", LK_FUNCSIG);   \
			}                                            \
			LK_DEBUG_BREAK();                            \
		}                                                \
	}
#else
#	define LK_ASSERT(Condition, ...) static_cast<void>(0)
#endif

/**
 * Verify.
 */
#ifdef LK_ENABLE_VERIFY
#	ifdef __LK_VA_OPT
#		define __LK_VERIFY_MESSAGE(...) ::platformer2d::CLog::PrintAssertMessage(::platformer2d::ELoggerType::Core, "Verify failed" __VA_OPT__(, ) __VA_ARGS__)
#	else
#		define __LK_VERIFY_MESSAGE(...) ::platformer2d::CLog::PrintAssertMessage(::platformer2d::ELoggerType::Core, "Verify failed", ##__VA_ARGS__)
#	endif
#define LK_VERIFY(Condition, ...)                             \
	{                                                         \
		if (!(Condition))                                     \
		{                                                     \
			if constexpr (sizeof(#__VA_ARGS__) > 1)           \
			{                                                 \
				__LK_VERIFY_MESSAGE(__VA_ARGS__);             \
			}                                                 \
			else                                              \
			{                                                 \
				__LK_VERIFY_MESSAGE("{}", LK_FUNCSIG);        \
			}                                                 \
			LK_DEBUG_BREAK();                                 \
		}                                                     \
	}
#else
#	define LK_VERIFY(Condition, ...) static_cast<void>(0)
#endif
