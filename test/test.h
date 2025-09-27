#pragma once

#include <filesystem>

#define __LK_TEST_STRINGIFY(x) #x

/**
 * @def LK_TEST_STRINGIFY
 * @brief Expand token before stringifying it.
 */
#define LK_TEST_STRINGIFY(x) __LK_TEST_STRINGIFY(x)

namespace platformer2d::test {

	std::filesystem::path GetBinaryDirectory();

}