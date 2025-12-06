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
#define LOAD_EDITOR_ON_STARTUP 1
#if LOAD_EDITOR_ON_STARTUP
#include "game/editor.h"
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

#ifdef LOAD_EDITOR_ON_STARTUP
		LK_TRACE_TAG("Application", "Adding testlevel to layerstack");
		std::shared_ptr<CEditor> Editor = std::make_shared<CEditor>();
		LayerStack.PushLayer(Editor);
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

			CEffectManager::Get().Destroy();
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

			Window->BeginFrame();
			CKeyboard::Update();

			CRenderer::BeginFrame();
			CPhysicsWorld::Update(DeltaTime);
			for (auto& Layer : LayerStack)
			{
				Layer->Tick(DeltaTime);
			}

			EffectManager.Tick(DeltaTime);

			/* Render UI. */
			{
#if 0
				CRenderer::Submit([&]() { RenderUI(); });
				CRenderer::Submit([&]() { UILayer->EndFrame(); });
#endif
			}

			CRenderer::EndFrame();
			CRenderer::Submit([&]() { Window->EndFrame(); });

#define NO_RENDER_SUBMISSION
#ifdef NO_RENDER_SUBMISSION
			RenderUI();
			UILayer->EndFrame();
#endif

			CKeyboard::TransitionPressedKeys();
		}
	}

	bool CApplication::PushLayer(std::shared_ptr<CLayer> Layer)
	{
		LK_ASSERT(Layer);
		return LayerStack.PushLayer(Layer);
	}

	void CApplication::RenderUI()
	{
		UILayer->BeginFrame();
		for (auto& Layer : LayerStack)
		{
			Layer->RenderUI();
		}
	}

}
