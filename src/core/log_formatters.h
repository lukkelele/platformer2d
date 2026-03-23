#pragma once

#include <string>
#include <filesystem>

#ifdef LK_COMPILER_MSVC
#	include <format>
#else
#	include <spdlog/fmt/fmt.h>
#endif

#include <lklog/lklog.h>
#include <glm/glm.hpp>

template<>
struct lklog::fmt::formatter<glm::vec2>
{
	template<typename ParseContext>
	constexpr auto parse(ParseContext& Context)
	{
		return Context.begin();
	}

	template<typename FormatContext>
	auto format(const glm::vec2& Input, FormatContext& Context) const
	{
		return lklog::fmt::format_to(Context.out(), "({:.2f}, {:.2f})", Input.x, Input.y);
	}
};

template<>
struct lklog::fmt::formatter<glm::vec3>
{
	template<typename ParseContext>
	constexpr auto parse(ParseContext& Context)
	{
		return Context.begin();
	}

	template<typename FormatContext>
	auto format(const glm::vec3& Input, FormatContext& Context) const
	{
		return lklog::fmt::format_to(Context.out(), "({:.2f}, {:.2f}, {:.2f})", Input.x, Input.y, Input.z);
	}
};

template<>
struct lklog::fmt::formatter<glm::vec4>
{
	template<typename ParseContext>
	constexpr auto parse(ParseContext& Context)
	{
		return Context.begin();
	}

	template<typename FormatContext>
	auto format(const glm::vec4& Input, FormatContext& Context) const
	{
		return lklog::fmt::format_to(Context.out(), "({:.2f}, {:.2f}, {:.2f}, {:.2f})", Input.x, Input.y, Input.z, Input.z);
	}
};

