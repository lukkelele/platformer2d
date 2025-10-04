#pragma once

#include <stdint.h>
#include <type_traits>

#define LK_UNUSED(Arg)       ((void)(Arg))
#define LK_ARRAYSIZE(Array)  (static_cast<int>((sizeof(Array) / sizeof(*(Array)))))
#define LK_STRINGIFY(x) #x

#if defined(LK_COMPILER_MSVC)
#	define FORCEINLINE __forceinline
#	define VARARGS     __cdecl
#	define STDCALL     __stdcall
#elif defined(LK_COMPILER_GCC)
#	define VARARGS		__attribute__((cdecl))
#	define FORCEINLINE	inline __attribute__((always_inline))
#	define STDCALL		__attribute__((stdcall))
#elif defined(LK_COMPILER_CLANG)
#	define FORCEINLINE  __forceinline
#	define VARARGS      __cdecl
#	define STDCALL      __stdcall
#endif

#ifdef SPDLOG_USE_STD_FORMAT
#	define LK_FMT std::format
#else
#	define LK_FMT fmt::format
#endif

namespace platformer2d {

	using LRendererID = uint32_t;

}