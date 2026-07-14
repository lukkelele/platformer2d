#pragma once

#include <chrono>

#ifdef LK_PROFILING
#	include <tracy/Tracy.hpp>

#	include "platform.h"
#	include "macros.h"

#	if LK_PROFILING_SCOPED_TIMERS
#		define LK_PROFILE_TIMER(...)                                                          \
			const ::platformer2d::Profiler::CScopedTimer LK_CONCAT(LkProfilerTimer_, __LINE__) \
			{                                                                                  \
				__func__ __VA_OPT__(, ) __VA_ARGS__                                            \
			}
#	else
#		define LK_PROFILE_TIMER(...)
#	endif /* LK_PROFILING_SCOPED_TIMERS */

#	define LK_PROFILE_MARK_FRAME()           FrameMark
#	define LK_PROFILE_MARK_FRAME_BEGIN(Name) FrameMarkStart(Name)
#	define LK_PROFILE_MARK_FRAME_END(Name)   FrameMarkEnd(Name)
#	define LK_PROFILE_FUNC(...)              ZoneScoped##__VA_OPT__(N(__VA_ARGS__))
#	define LK_PROFILE_THREAD(...)            tracy::SetThreadName(__VA_ARGS__)

#else /* PROFILING DISABLED */
#	define LK_PROFILE_TIMER(...)
#	define LK_PROFILE_MARK_FRAME()
#	define LK_PROFILE_MARK_FRAME_BEGIN(Name)
#	define LK_PROFILE_MARK_FRAME_END(Name)
#	define LK_PROFILE_FUNC(...)
#	define LK_PROFILE_THREAD(...)
#endif /* LK_PROFILING */

namespace platformer2d::Profiler {
	void PrintLine(std::string_view Function, std::string_view Message, std::chrono::nanoseconds Elapsed);

	class CScopedTimer
	{
	public:
		explicit CScopedTimer(const char* InFunction, std::string_view InMessage = {})
			: Function(InFunction)
			, Message(InMessage)
			, Start(std::chrono::steady_clock::now())
		{
		}
		CScopedTimer(CScopedTimer&&) = delete;
		CScopedTimer(const CScopedTimer&) = delete;
		~CScopedTimer()
		{
			const auto Elapsed = std::chrono::steady_clock::now() - Start;
			PrintLine(Function, Message, std::chrono::duration_cast<std::chrono::nanoseconds>(Elapsed));
		}

		CScopedTimer& operator=(const CScopedTimer&) = delete;
		CScopedTimer& operator=(CScopedTimer&&) = delete;

	private:
		const char* Function = nullptr;
		std::string_view Message;
		std::chrono::steady_clock::time_point Start;
	};
}
