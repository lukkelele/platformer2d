#pragma once

#include <filesystem>
#include <memory>

#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <spdlog/spdlog.h>

#include "lk_config.h"
#include "core/assert.h"
#include "core/window.h"
#include "renderer/backendinfo.h"

#define __LK_TEST_STRINGIFY(x) #x

/**
 * @def LK_TEST_STRINGIFY
 * @brief Expand token before stringifying it.
 */
#define LK_TEST_STRINGIFY(x) __LK_TEST_STRINGIFY(x)

namespace platformer2d::test {

	class CTestBase
	{
	protected:
		explicit CTestBase(int Argc, char* Argv[]);
	public:
		CTestBase() = delete;
		virtual ~CTestBase() = default;

		virtual void Run() = 0;
		virtual void Stop();
		virtual void Destroy() = 0;

		bool IsRunning() const { return Running; }
		const CWindow& GetWindow() const { return *Window; }
		const std::filesystem::path& GetBinaryDirectory() const { return BinaryDir; }
		static const std::filesystem::path& GetAssetsDirectory() { return AssetsDir; }

		static void InitRenderContext(GLFWwindow* GlfwWindow);
		static void ImGui_NewFrame();
		static void ImGui_EndFrame();

	protected:
		bool Running = false;
		struct {
			int Argc;
			char** Argv;
		} Args;

		OpenGL::FBackendInfo BackendInfo;

		std::unique_ptr<CWindow> Window;
		std::filesystem::path BinaryDir{};

		static std::filesystem::path AssetsDir;
	};

}