#pragma once

#ifdef LK_PROFILING
#	include "tracy/Tracy.hpp"

#	include "platform.h"
#	include "timer.h"
#	include "macros.h"

#	if LK_PROFILING_USE_SCOPED_TIMERS
#		define LK_PROFILE_SCOPE(Name) CScopedTimer LK_CONCAT_EXPAND(__LK_SCOPE_TIMER_, __LINE__)(Name);
#	else
#		define LK_PROFILE_SCOPE(Name) /* @todo: Tracy impl of some sort */
#	endif /* LK_PROFILING_USE_SCOPED_TIMERS */

#	define LK_PROFILE_MARK_FRAME()           FrameMark
#	define LK_PROFILE_MARK_FRAME_BEGIN(Name) FrameMarkStart(Name)
#	define LK_PROFILE_MARK_FRAME_END(Name)   FrameMarkEnd(Name)
#	define LK_PROFILE_FUNC(...)              ZoneScoped##__VA_OPT__(N(__VA_ARGS__))
#	define LK_PROFILE_THREAD(...)            tracy::SetThreadName(__VA_ARGS__)

#else /* PROFILING DISABLED */
#	define LK_PROFILE_SCOPE(Name)
#	define LK_PROFILE_MARK_FRAME()
#	define LK_PROFILE_MARK_FRAME_BEGIN(Name)
#	define LK_PROFILE_MARK_FRAME_END(Name)
#	define LK_PROFILE_FUNC(...)
#	define LK_PROFILE_THREAD(...)
#endif /* LK_PROFILING */

