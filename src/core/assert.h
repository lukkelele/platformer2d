#pragma once

#include <cassert>
#include <cstdio>

#if defined(LK_COMPILER_MSVC)
#	define LK_DEBUG_BREAK __debugbreak()
#elif defined(LK_COMPILER_CLANG)
#	define LK_DEBUG_BREAK __builtin_debugtrap()
#elif defined(LK_COMPILER_GCC)
#	define LK_DEBUG_BREAK __builtin_trap()
#endif

#if defined(LK_COMPILER_MSVC)
#	define LK_FUNCSIG __FUNCSIG__
#elif defined(LK_COMPILER_GCC) || defined(LK_COMPILER_CLANG)
#	define LK_FUNCSIG __PRETTY_FUNCTION__
#else
#	error "Unsupported compiler"
#endif

/**
 * Assert.
 */
#if LK_ENABLE_ASSERT
#define LK_ASSERT_MESSAGE_INTERNAL(...) std::printf("Verify Failed: %s\nLine: %d\n", LK_FUNCSIG, __LINE__)
#define LK_ASSERT(Condition, ...)                         \
		{                                                    \
			if (!(Condition))                                \
			{                                                \
				std::printf("Assert Failed: %s\nLine: %d\n", LK_FUNCSIG, __LINE__); \
				LK_DEBUG_BREAK;                              \
			}                                                \
		}
#else
#	define LK_ASSERT(Condition, ...) static_cast<void>(0)
#endif

/**
 * Verify.
 */
#if LK_ENABLE_VERIFY
#define LK_VERIFY_MESSAGE_INTERNAL(...) std::printf("Verify Failed: %s\nLine: %d\n", LK_FUNCSIG, __LINE__)
#define LK_VERIFY(Condition, ...)                              \
	{                                                         \
		if (!(Condition))                                     \
		{                                                     \
			std::printf("Verify Failed: %s\nLine: %d\n", LK_FUNCSIG, __LINE__); \
			LK_DEBUG_BREAK;                                   \
		}                                                     \
	}
#else
#	define LK_VERIFY(Condition, ...) static_cast<void>(0)
#endif
