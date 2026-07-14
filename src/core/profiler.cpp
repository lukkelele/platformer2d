#include "profiler.h"

#include <format>
#include <print>

#include "ansi.h"

namespace platformer2d::Profiler {
	static constexpr bool COLORED = true;
	static constexpr std::size_t FUNC_WIDTH = 24;
	static constexpr std::size_t MSG_WIDTH = 22;
	static constexpr std::string_view PERFECT_PERFORMANCE_COLOR = ANSI::GREEN;
	static constexpr std::string_view GOOD_PERFORMANCE_COLOR = ANSI::WHITE;
	static constexpr std::string_view WORRYSOME_PERFORMANCE_COLOR = ANSI::YELLOW;
	static constexpr std::string_view BAD_PERFORMANCE_COLOR = ANSI::RED;

	static void SetLineFormat(double& Time, const bool TimeMicroseconds, std::string_view& Unit, std::string_view& Color)
	{
		if (TimeMicroseconds) {
			Unit = "us";
			if (Time <= 5) {
				Color = PERFECT_PERFORMANCE_COLOR;
			} else {
				Color = GOOD_PERFORMANCE_COLOR;
			}
		} else {
			if (Time >= 1000) {
				Unit = "s";
				Color = BAD_PERFORMANCE_COLOR;
				Time /= 1000.0;
			} else if ((Time >= 500)) {
				Unit = "ms";
				Color = BAD_PERFORMANCE_COLOR;
			} else {
				Unit = "ms";
				Color = WORRYSOME_PERFORMANCE_COLOR;
			}
		}
	}

	void PrintLine(const std::string_view Function, const std::string_view Message, const std::chrono::nanoseconds Elapsed)
	{
		const auto Ns = Elapsed.count();
		const bool Micros = Ns < 1'000'000;
		double Value = Micros ? (static_cast<double>(Ns) / 1000.0) : (static_cast<double>(Ns) / 1'000'000.0);
		std::string_view Unit;
		std::string_view Color;
		SetLineFormat(Value, Micros, Unit, Color);

		const std::string FuncColor = std::format("[{:<{}}]", Function, FUNC_WIDTH);
		const std::string MsgColor = Message.empty() ? "" : std::format("|  {}", Message);
		const std::string TimeColor = std::format("[{:>8.3f} {:>2}]", Value, Unit);
		if (COLORED) {
			std::println("{}  {}{}{}  {}", FuncColor, Color, TimeColor, ANSI::RESET, MsgColor);
		} else {
			std::println("{}  {}  {}", FuncColor, MsgColor, TimeColor);
		}
	}
}
