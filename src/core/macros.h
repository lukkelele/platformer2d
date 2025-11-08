#pragma once

#include "lk_config.h"

#define LK_UNUSED(...)       (void)(sizeof(__VA_ARGS__))
#define LK_ARRAYSIZE(Array)  (static_cast<int>((sizeof(Array) / sizeof(*(Array)))))
#define LK_STRINGIFY(x)      #x
#define LK_BIT(x)            (1 << x)

#if defined(FORCEINLINE) || defined(CDECL) || defined(STDCALL)
#error "Already defined"
#endif

#if defined(LK_COMPILER_MSVC)
#	define FORCEINLINE  __forceinline
#	define CDECL        __cdecl
#	define STDCALL      __stdcall
#elif defined(LK_COMPILER_GCC)
#	define FORCEINLINE	inline __attribute__((always_inline))
#	define CDECL        __attribute__((cdecl))
#	define STDCALL		__attribute__((stdcall))
#elif defined(LK_COMPILER_CLANG)
#	define FORCEINLINE  __forceinline
#	define CDECL        __cdecl
#	define STDCALL      __stdcall
#endif

#ifdef SPDLOG_USE_STD_FORMAT
#	define LK_FMT std::format
#	define LK_FMT_LIB std
#else
#	define LK_FMT fmt::format
#	define LK_FMT_LIB fmt
#endif
