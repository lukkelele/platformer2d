#include "application.h"

#include "core/input/keyboard.h"
#include "core/input/mouse.h"
#include "game/player.h"
#include "physics/physicsworld.h"
#include "physics/body.h"
#include "renderer/debugrenderer.h"
#include "renderer/vertexbufferlayout.h"
#include "renderer/ui/ui.h"
#include "renderer/ui/uilayer.h"
#include "scene/effectmanager.h"

/** @todo: Level loading from a main menu */
#define LOAD_TEST_LEVEL_ON_STARTUP 1
#if LOAD_TEST_LEVEL_ON_STARTUP
#include "game/level/testlevel.h"
#endif

namespace platformer2d {

	CApplication::CApplication(int Argc, char* Argv[])
	{
		CLog::Initialize();
	}

	CApplication::~CApplication()
	{
		Shutdown();
	}

	void CApplication::Initialize()
	{
		LK_DEBUG_TAG("Application", "Initializing");
		const char* WindowName = "platformer2d";
		Window = std::make_unique<CWindow>(SCREEN_WIDTH, SCREEN_HEIGHT, WindowName);
		Window->Initialize();

		CPhysicsWorld::Initialize();
		CRenderer::Initialize();
		CKeyboard::Initialize();
		CMouse::Initialize();

		LK_TRACE_TAG("Application", "Adding UI layer to layerstack");
		UILayer = std::make_shared<CUILayer>();
		LayerStack.PushOverlay(UILayer);

#ifdef LOAD_TEST_LEVEL_ON_STARTUP
		LK_TRACE_TAG("Application", "Adding testlevel to layerstack");
		std::shared_ptr<Level::CTestLevel> TestLevel = std::make_shared<Level::CTestLevel>();
		LayerStack.PushLayer(TestLevel);
#endif
	}

	void CApplication::Shutdown()
	{
		if (bRunning)
		{
			LK_INFO_TAG("Application", "Shutting down");
			bRunning = false;

			UILayer.reset();
			LK_TRACE_TAG("Application", "Release layerstack");
			LayerStack.Destroy();

			CRenderer::Destroy();

			Window->Destroy();
			Window.reset();
		}
	}

	void CApplication::Run()
	{
		LK_VERIFY(Window && Window->GetGlfwWindow());
		LK_VERIFY(LayerStack.Count() > 0);

		const FWindowData& WindowData = Window->GetData();
		GLFWwindow* GlfwWindow = Window->GetGlfwWindow();

		CEffectManager& EffectManager = CEffectManager::Get(); /* @todo Integrate in layerstack somehow (?) */

		bRunning = true;
		Timer.Reset();
		while (!glfwWindowShouldClose(GlfwWindow))
		{
			if (Core::Global.bShouldShutdown)
			{
				break;
			}

			const float DeltaTime = Timer.GetDeltaTime();
			CPhysicsWorld::Update(DeltaTime);

			Window->BeginFrame();
			CKeyboard::Update();
			UILayer->BeginFrame();
			CRenderer::BeginFrame();

			for (auto& Layer : LayerStack)
			{
				Layer->Tick(DeltaTime);
			}

			EffectManager.Tick(DeltaTime);

			/* Render UI. */
			for (auto& Layer : LayerStack)
			{
				Layer->RenderUI();
			}

			CRenderer::EndFrame();
			UILayer->EndFrame();
			CKeyboard::TransitionPressedKeys();
			Window->EndFrame();
		}
	}

	bool CApplication::PushLayer(std::shared_ptr<CLayer> Layer)
	{
		LK_VERIFY(Layer);
		return LayerStack.PushLayer(Layer);
	}

}
