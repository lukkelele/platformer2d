#pragma once

#include <string>

#include <spdlog/spdlog.h>

#define LK_TRACE(...) spdlog::trace(__VA_ARGS__)
#define LK_DEBUG(...) spdlog::debug(__VA_ARGS__)
#define LK_INFO(...)  spdlog::info(__VA_ARGS__)
#define LK_WARN(...)  spdlog::warn(__VA_ARGS__)
#define LK_ERROR(...)  spdlog::error(__VA_ARGS__)
