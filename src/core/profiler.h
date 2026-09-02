#pragma once

#include <chrono>

#ifdef LK_PROFILING
#include <tracy/Tracy.hpp>

#include "platform.h"
#include "macros.h"

#ifndef LK_PROFILING_SCOPED_TIMERS
#define LK_PROFILER_SCOPED(...) ZoneScoped##__VA_OPT__(N(__VA_ARGS__))
#else
#define LK_PROFILER_TIMER(...)                                                         \
	const ::platformer2d::Profiler::CScopedTimer LK_CONCAT(LkProfilerTimer_, __LINE__) \
	{                                                                                  \
		__func__ __VA_OPT__(, ) __VA_ARGS__                                            \
	}
#define LK_PROFILER_SCOPED(...) LK_PROFILER_TIMER(__VA_ARGS__)
#endif /* LK_PROFILING_SCOPED_TIMERS */

#define LK_PROFILER_MARK_FRAME()                                    FrameMark
#define LK_PROFILER_MARK_FRAME_BEGIN(_name)                         FrameMarkStart(_name)
#define LK_PROFILER_MARK_FRAME_END(_name)                           FrameMarkEnd(_name)
#define LK_PROFILER_THREAD(...)                                     tracy::SetThreadName(__VA_ARGS__)
#define LK_PROFILER_LOCKABLE(_type, _var)                           TracyLockable(_type, _var)
#define LK_PROFILER_LOCKABLE_N(_type, _var, _desc)                  TracyLockableN(_type, _var, _desc)
#define LK_PROFILER_LOCKABLE_BASE(_type)                            LockableBase(_type)
#define LK_PROFILER_PLOT(_name, _value)                             TracyPlot(_name, _value)
#define LK_PROFILER_PLOT_CONFIG(_name, _type, _step, _fill, _color) TracyPlotConfig(_name, _type, _step, _fill, _color)
#define LK_PROFILER_MESSAGE(_txt, _size)                            TracyMessage(_txt, _size)
#define LK_PROFILER_MESSAGE_L(_txt)                                 TracyMessageL(_txt)
#else /* PROFILING DISABLED */
#define LK_PROFILER_SCOPED(...)
#define LK_PROFILER_MARK_FRAME()
#define LK_PROFILER_MARK_FRAME_BEGIN(_name)
#define LK_PROFILER_MARK_FRAME_END(_name)
#define LK_PROFILER_THREAD(...)
#define LK_PROFILER_LOCKABLE(_type, _var)          _type _var
#define LK_PROFILER_LOCKABLE_N(_type, _var, _desc) _type _var
#define LK_PROFILER_LOCKABLE_BASE(_type)           _type
#define LK_PROFILER_PLOT(_name, _value)
#define LK_PROFILER_PLOT_CONFIG(_name, _type, _step, _fill, _color)
#define LK_PROFILER_MESSAGE(_txt, _size)
#define LK_PROFILER_MESSAGE_L(_txt)
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
