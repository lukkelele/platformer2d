#pragma once

#include <string>
#include <filesystem>

#ifdef LK_COMPILER_MSVC
#include <format>
#else
#include <spdlog/fmt/fmt.h>
#endif

#include <glm/glm.hpp>

#include "macros.h"

template<>
struct LK_FMT_LIB::formatter<std::filesystem::path> : LK_FMT_LIB::formatter<std::string>
{
    template<typename FormatContext>
    auto format(const std::filesystem::path& Input, FormatContext& Context) const
    {
        return LK_FMT_LIB::format_to(Context.out(), "{}", Input.generic_string());
    }
};

template<>
struct LK_FMT_LIB::formatter<glm::vec2>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& Context)
    {
        return Context.begin();
    }
	
    template<typename FormatContext>
    auto format(const glm::vec2& Input, FormatContext& Context) const
    {
        return LK_FMT_LIB::format_to(Context.out(), "({:.2f}, {:.2f})", Input.x, Input.y);
    }
};

template<>
struct LK_FMT_LIB::formatter<glm::vec3>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& Context)
    {
        return Context.begin();
    }

    template<typename FormatContext>
    auto format(const glm::vec3& Input, FormatContext& Context) const
    {
        return LK_FMT_LIB::format_to(Context.out(), "({:.2f}, {:.2f}, {:.2f})", Input.x, Input.y, Input.z);
    }
};

template<>
struct LK_FMT_LIB::formatter<glm::vec4>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& Context)
    {
        return Context.begin();
    }

    template<typename FormatContext>
    auto format(const glm::vec4& Input, FormatContext& Context) const
    {
        return LK_FMT_LIB::format_to(Context.out(), "({:.2f}, {:.2f}, {:.2f}, {:.2f})", Input.x, Input.y, Input.z, Input.z);
    }
};
