#pragma once

#include <filesystem>
#include <memory>

#include "core/window.h"

#define __LK_TEST_STRINGIFY(x) #x

/**
 * @def LK_TEST_STRINGIFY
 * @brief Expand token before stringifying it.
 */
#define LK_TEST_STRINGIFY(x) __LK_TEST_STRINGIFY(x)

struct GLFWwindow;

namespace platformer2d::test {

	class CTest
	{
	public:
		CTest();
		virtual ~CTest() = default;

		const CWindow& GetWindow() const { return *Window; }
		const std::filesystem::path& GetBinaryDirectory() const { return BinaryDir; }

		static void InitRenderContext(GLFWwindow* GlfwWindow);
		static void ImGui_NewFrame();
		static void ImGui_EndFrame();

	protected:
		std::unique_ptr<CWindow> Window;
		std::filesystem::path BinaryDir{};

		static std::filesystem::path AssetsDir;
	};

}