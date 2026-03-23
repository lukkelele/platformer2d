#pragma once

#include "lk_config.h"

#define LK_UNUSED(...)      (void)(sizeof(__VA_ARGS__))
#define LK_ARRAYSIZE(Array) (static_cast<int>((sizeof(Array) / sizeof(*(Array)))))
#define LK_STRINGIFY(x)     #x
#define LK_BIT(x)           (1 << x)

#define LK_CONCAT(a, b)        a##b
#define LK_CONCAT_EXPAND(a, b) LK_CONCAT(a, b)

#if defined(LK_COMPILER_MSVC)
#	define FORCEINLINE __forceinline
#	define LK_CDECL    __cdecl
#	define LK_STDCALL  __stdcall
#elif defined(LK_COMPILER_GCC)
#	define FORCEINLINE inline __attribute__((always_inline))
#	define LK_CDECL    __attribute__((cdecl))
#	define LK_STDCALL  __attribute__((stdcall))
#elif defined(LK_COMPILER_CLANG)
#	define FORCEINLINE __forceinline
#	define LK_CDECL    __cdecl
#	define LK_STDCALL  __stdcall
#endif

