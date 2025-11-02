#include "application.h"

#include "game/player.h"
#include "core/input/keyboard.h"
#include "core/input/mouse.h"
#include "renderer/debugrenderer.h"
#include "renderer/vertexbufferlayout.h"
#include "renderer/ui.h"
#include "physics/physicsworld.h"
#include "physics/body.h"

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
	}

	void CApplication::Shutdown()
	{
		if (bRunning)
		{
			LK_INFO_TAG("Application", "Shutting down");
			bRunning = false;

			Window->Destroy();
			Window.release();

			LK_TRACE_TAG("Application", "Release layerstack");
			LayerStack.Destroy();
		}
	}

	void CApplication::Run()
	{
		LK_VERIFY(Window && Window->GetGlfwWindow());
		LK_VERIFY(LayerStack.Count() > 0);

		const FWindowData& WindowData = Window->GetData();
		GLFWwindow* GlfwWindow = Window->GetGlfwWindow();

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
			CRenderer::BeginFrame();

			for (auto& Layer : LayerStack)
			{
				Layer->Tick(DeltaTime);
			}

			/* Render UI. */
			for (auto& Layer : LayerStack)
			{
				Layer->RenderUI();
			}

			CRenderer::EndFrame();
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
