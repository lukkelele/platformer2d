#pragma once

#include <string>
#include <filesystem>
#include <format>

#include <glm/glm.hpp>

template<>
struct std::formatter<std::filesystem::path> : std::formatter<std::string>
{
	template<typename FormatContext>
    auto format(const std::filesystem::path& Input, FormatContext& Context) const
    {
        return std::format_to(Context.out(), "{}", Input.generic_string());
    }
};

template<>
struct std::formatter<glm::vec2>
{
	template<typename ParseContext>
    constexpr auto parse(ParseContext& Context)
    {
        return Context.begin();
    }
	
	template<typename FormatContext>
    auto format(const glm::vec2& Input, FormatContext& Context) const
    {
        return std::format_to(Context.out(), "({:.2f}, {:.2f})", Input.x, Input.y);
    }
};

template<>
struct std::formatter<glm::vec3>
{
	template<typename ParseContext>
    constexpr auto parse(ParseContext& Context)
    {
        return Context.begin();
    }
	
	template<typename FormatContext>
    auto format(const glm::vec3& Input, FormatContext& Context) const
    {
        return std::format_to(Context.out(), "({:.2f}, {:.2f}, {:.2f})", Input.x, Input.y, Input.z);
    }
};

template<>
struct std::formatter<glm::vec4>
{
	template<typename ParseContext>
    constexpr auto parse(ParseContext& Context)
    {
        return Context.begin();
    }
	
	template<typename FormatContext>
    auto format(const glm::vec4& Input, FormatContext& Context) const
    {
        return std::format_to(Context.out(), "({:.2f}, {:.2f}, {:.2f}, {:.2f})", Input.x, Input.y, Input.z, Input.z);
    }
};
