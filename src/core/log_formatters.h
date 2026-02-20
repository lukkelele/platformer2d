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
struct LkFmt::formatter<std::filesystem::path> : LkFmt::formatter<std::string>
{
    template<typename FormatContext>
    auto format(const std::filesystem::path& Input, FormatContext& Context) const
    {
        return LkFmt::format_to(Context.out(), "{}", Input.generic_string());
    }
};

template<>
struct LkFmt::formatter<glm::vec2>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& Context)
    {
        return Context.begin();
    }

    template<typename FormatContext>
    auto format(const glm::vec2& Input, FormatContext& Context) const
    {
        return LkFmt::format_to(Context.out(), "({:.2f}, {:.2f})", Input.x, Input.y);
    }
};

template<>
struct LkFmt::formatter<glm::vec3>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& Context)
    {
        return Context.begin();
    }

    template<typename FormatContext>
    auto format(const glm::vec3& Input, FormatContext& Context) const
    {
        return LkFmt::format_to(Context.out(), "({:.2f}, {:.2f}, {:.2f})", Input.x, Input.y, Input.z);
    }
};

template<>
struct LkFmt::formatter<glm::vec4>
{
    template<typename ParseContext>
    constexpr auto parse(ParseContext& Context)
    {
        return Context.begin();
    }

    template<typename FormatContext>
    auto format(const glm::vec4& Input, FormatContext& Context) const
    {
        return LkFmt::format_to(Context.out(), "({:.2f}, {:.2f}, {:.2f}, {:.2f})", Input.x, Input.y, Input.z, Input.z);
    }
};
