#include "test.h"

#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>
#include <stb/stb_image.h>

#include "core/application.h"
#include "core/assert.h"
#include "core/window.h"
#include "core/input/keyboard.h"
#include "renderer/opengl.h"

#define LOAD_TEST_LEVEL_ON_STARTUP 0
#if LOAD_TEST_LEVEL_ON_STARTUP
#include "game/level/testlevel.h"
#endif

namespace platformer2d::test {

	namespace
	{
		std::unique_ptr<CApplication> Application;
		constexpr bool NO_TEST_INIT = false;
	}

	CTest::CTest(const int Argc, char* Argv[])
		: CTestBase(Argc, Argv, NO_TEST_INIT)
	{
		Application = std::make_unique<CApplication>(Argc, Argv);
	}

	void CTest::Run()
	{
		Application->Initialize();

		bRunning = true;
#ifdef RUN_CATCH_TESTS
		const int CatchResult = Catch::Session().run(Args.Argc, Args.Argv);
#endif

#if LOAD_TEST_LEVEL_ON_STARTUP
		std::shared_ptr<Level::CEditor> TestLevel = std::make_shared<Level::CEditor>();
		const bool LayerAdded = Application->PushLayer(TestLevel);
		LK_VERIFY(LayerAdded, "Failed to add layer");
#endif

		Application->Run();
	}

	void CTest::Destroy()
	{
		LK_DEBUG_TAG("Test", "Destroy");
		Application->Shutdown();
	}

}