#pragma once

#include <filesystem>
#include <memory>

#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>

#include "lk_config.h"
#include "core/assert.h"
#include "core/log.h"
#include "core/window.h"
#include "renderer/backendinfo.h"

#ifndef LK_TEST_SUITE
#	error "LK_TEST_SUITE not defined"
#endif

#ifndef LK_TEST_NAME
#	error "LK_TEST_NAME not defined"
#endif

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
		explicit CTestBase(int Argc, char* Argv[], bool bInit = true);

	public:
		CTestBase() = delete;
		virtual ~CTestBase() = default;

		virtual void Run() = 0;
		virtual void Stop();
		virtual void Destroy() = 0;

		[[nodiscard]] bool IsRunning() const { return bRunning; }
		[[nodiscard]] const std::filesystem::path& GetBinaryDirectory() const { return BinaryDir; }
		[[nodiscard]] static const std::filesystem::path& GetAssetsDirectory() { return AssetsDir; }

		static void InitRenderContext(GLFWwindow* GlfwWindow);
		static void ImGui_NewFrame();
		static void ImGui_EndFrame();

		static bool UI_BlendFunction();

	protected:
		bool bRunning = false;
		struct
		{
			int Argc;
			char** Argv;
		} Args;

		FBackendInfo BackendInfo;

		std::filesystem::path BinaryDir;
		static inline std::filesystem::path AssetsDir = ASSETS_DIR;
	};
}

