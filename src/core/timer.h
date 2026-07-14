#pragma once

#include <cstddef>
#include <cstdio>
#include <chrono>
#include <type_traits>

#include "log.h"

namespace platformer2d {
	using namespace std::chrono_literals;

	class CTimer
	{
	public:
		CTimer() { Reset(); }
		CTimer(CTimer&&) = delete;
		CTimer(const CTimer&) = delete;
		~CTimer() = default;

		CTimer& operator=(CTimer&&) = delete;
		CTimer& operator=(const CTimer&) = delete;

		void Reset()
		{
			StartTime = std::chrono::high_resolution_clock::now();
			LastTime = StartTime;
		}

		template<typename TDuration>
		[[nodiscard]] TDuration GetElapsed() const
		{
			using namespace std::chrono;
			static_assert(std::disjunction_v<
							  std::is_same<TDuration, microseconds>,
							  std::is_same<TDuration, milliseconds>,
							  std::is_same<TDuration, seconds>>,
				"Timer format not supported");
			return duration_cast<TDuration>(high_resolution_clock::now() - StartTime);
		}

		[[nodiscard]] float GetDeltaTime()
		{
			using namespace std::chrono;
			const auto Now = high_resolution_clock::now();
			const duration<float> DeltaTime = Now - LastTime;
			LastTime = Now;
			return DeltaTime.count();
		}

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> StartTime{};
		std::chrono::time_point<std::chrono::high_resolution_clock> LastTime{};
	};
}
