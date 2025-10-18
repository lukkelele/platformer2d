#pragma once

#if !defined(LK_COMPILER_MSVC) && !defined(LK_COMPILER_GCC) && !defined(LK_COMPILER_CLANG)
#error "Missing supported compiler"
#endif

/* Debug breakpoint. */
#ifndef LK_DEBUG_BREAK
#if defined(LK_COMPILER_MSVC)
#	define LK_DEBUG_BREAK() __debugbreak()
#elif defined(LK_COMPILER_CLANG)
#	define LK_DEBUG_BREAK() __builtin_debugtrap()
#elif defined(LK_COMPILER_GCC)
#	define LK_DEBUG_BREAK() __builtin_trap()
#endif
#endif /* LK_DEBUG_BREAK */

/* Function signature. */
#ifndef LK_FUNCSIG
#if defined(LK_COMPILER_MSVC)
#	define LK_FUNCSIG __FUNCSIG__
#elif defined(LK_COMPILER_GCC) || defined(LK_COMPILER_CLANG)
#	define LK_FUNCSIG __PRETTY_FUNCTION__
#else
#	error "Unsupported compiler"
#endif
#endif /* LK_FUNCSIG */
