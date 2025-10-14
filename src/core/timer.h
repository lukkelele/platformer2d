#pragma once

#include <cstddef>
#include <cstdio>
#include <chrono>
#include <type_traits>

namespace platformer2d {

	using namespace std::chrono_literals;

	class CTimer
	{
	public:
		CTimer() 
		{ 
			Reset(); 
		}

		virtual ~CTimer() = default;

		void Reset() 
		{ 
			StartTime = std::chrono::high_resolution_clock::now(); 
			LastTime = StartTime;
		}

		template<typename TDuration>
		TDuration GetElapsed() const
		{
			using namespace std::chrono;
			static_assert(std::disjunction_v<
							  std::is_same<TDuration, microseconds>,
							  std::is_same<TDuration, milliseconds>,
							  std::is_same<TDuration, seconds>>, "Timer format not supported");
			return duration_cast<TDuration>(high_resolution_clock::now() - StartTime);
		}

		inline float GetDeltaTime()
		{
			using namespace std::chrono;
			const time_point<high_resolution_clock> Now = high_resolution_clock::now();
			const duration<float, std::milli> DeltaTime = Now - LastTime;
			LastTime = Now;

			return DeltaTime.count();
		}

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> StartTime{};
		std::chrono::time_point<std::chrono::high_resolution_clock> LastTime{};
	};

}