#include "application.h"

#include "core/profiler.h"
#include "core/input/keyboard.h"
#include "core/input/mouse.h"
#include "core/thread.h"
#include "game/editor.h"
#include "game/player.h"
#include "game/runtimelayer.h"
#include "physics/physicsworld.h"
#include "physics/body.h"
#include "renderer/debugrenderer.h"
#include "renderer/vertexbufferlayout.h"
#include "renderer/ui/ui.h"
#include "renderer/ui/uilayer.h"
#include "scene/effectmanager.h"

#define LOAD_EDITOR_ON_STARTUP 0

namespace platformer2d {

	static const std::filesystem::path SettingsFile(CONFIG_DIR "/settings.yaml");

	CApplication::CApplication(int Argc, char* Argv[])
	{
		lklog::init(lklog::level::debug, "platformer2d");
		Thread::Init();
	}

	CApplication::~CApplication()
	{
		Shutdown();
	}

	void CApplication::Initialize()
	{
		LK_DEBUG_TAG("Application", "Initializing");
		FSettings& Settings = FSettings::Get();
		Settings.Deserialize(SettingsFile);

		FWindowSpecification WindowSpec = {
			.Title = "platformer2d",
			.bStartMaximized = Settings.Window.bStartMaximized,
			.bVSync = Settings.Window.bVSync,
		};
		CWindow::Get().Initialize(WindowSpec);

		CRenderer::Initialize();
		CKeyboard::Initialize();
		CMouse::Initialize();

		LK_TRACE_TAG("Application", "Adding UI layer to layerstack");
		UILayer = std::make_shared<CUILayer>();
		UILayer->Initialize();
		LayerStack.PushOverlay(UILayer);

#if LOAD_EDITOR_ON_STARTUP
		LK_TRACE_TAG("Application", "Adding testlevel to layerstack");
		std::shared_ptr<CEditor> Editor = std::make_shared<CEditor>();
		LayerStack.PushLayer(Editor);
#endif
	}

	void CApplication::Shutdown()
	{
		if (bRunning) {
			LK_INFO_TAG("Application", "Shutting down");
			bRunning = false;

			const FSettings& Settings = FSettings::Get();
			Settings.Serialize(SettingsFile);

			UILayer.reset();
			LK_TRACE_TAG("Application", "Release layerstack");
			LayerStack.Destroy();

			CEffectManager::Get().Destroy();
			CRenderer::Destroy();

			CWindow::Get().Destroy();
		}
	}

	void CApplication::Run()
	{
		auto& Window = CWindow::Get();
		LK_VERIFY(Window.GetGlfwWindow());
		LK_VERIFY(LayerStack.Count() > 0);

		const FWindowData& WindowData = Window.GetData();
		GLFWwindow* GlfwWindow = Window.GetGlfwWindow();

		CEffectManager& EffectManager = CEffectManager::Get(); /* @todo Integrate in layerstack somehow (?) */

		LK_PROFILE_THREAD("Main");

		bRunning = true;
		Timer.Reset();
		while (!glfwWindowShouldClose(GlfwWindow)) {
			LK_PROFILE_FUNC("MainLoop");
			if (Core::Global.bShouldShutdown) {
				break;
			}

			CheckLayerQueues();

			const float DeltaTime = Timer.GetDeltaTime();

			Window.BeginFrame();
			CKeyboard::Update();
			CRenderer::BeginFrame();

			CPhysicsWorld::Update(DeltaTime);
			{
				LK_PROFILE_FUNC("LayerStack::Tick");
				for (auto& Layer : LayerStack) {
					Layer->Tick(DeltaTime);
				}
			}
			EffectManager.Tick(DeltaTime);

			CRenderer::EndFrame();
			RenderUI();

			CKeyboard::TransitionPressedKeys();
			Window.EndFrame();

			LK_PROFILE_MARK_FRAME();
		}
	}

	bool CApplication::PushLayer(std::shared_ptr<CLayer> Layer)
	{
		LK_ASSERT(Layer);
		return LayerStack.PushLayer(Layer);
	}

	void CApplication::RenderUI()
	{
		LK_PROFILE_FUNC();
		UILayer->BeginFrame();
		for (auto& Layer : LayerStack) {
			Layer->RenderUI();
		}
		UILayer->EndFrame();
	}

	void CApplication::CheckLayerQueues()
	{
		/* Queue: Layer addition */
		while (!Core::Global.LayerAddQueue.empty()) {
			const Core::ELayer LayerType = Core::Global.LayerAddQueue.front();
			LK_INFO_TAG("Application", "Adding layer: {}", Enum::ToString(LayerType));
			switch (LayerType) {
				case Core::ELayer::Runtime:
				{
					LK_VERIFY(!LayerStack.HasLayer("Runtime"), "Runtime layer already present");
					LK_VERIFY(!LayerStack.HasLayer("Editor"), "Editor layer present");
					std::shared_ptr<CRuntimeLayer> RuntimeLayer = std::make_shared<CRuntimeLayer>();
					LayerStack.PushLayer(RuntimeLayer);
					break;
				}
				case Core::ELayer::Editor:
				{
					LK_VERIFY(!LayerStack.HasLayer("Editor"), "Editor layer already present");
					LK_VERIFY(!LayerStack.HasLayer("Runtime"), "Runtime layer present");
					std::shared_ptr<CEditor> Editor = std::make_shared<CEditor>();
					LayerStack.PushLayer(Editor);
					break;
				}
				default:
					LK_VERIFY(false);
					break;
			}

			Core::Global.LayerAddQueue.pop();
		}

		/* Queue: Layer removal */
		while (!Core::Global.LayerRemoveQueue.empty()) {
			const Core::ELayer LayerType = Core::Global.LayerRemoveQueue.front();
			switch (LayerType) {
				case Core::ELayer::Runtime:
					if (std::shared_ptr<CLayer> Layer = LayerStack.GetLayer("Runtime")) {
						LK_INFO_TAG("Application", "Removing layer: {}", Enum::ToString(LayerType));
						LayerStack.PopLayer(Layer);
						CRenderer::SetClearColor(FColor::Convert(RGBA32::DarkerGray));
					}
					break;
				case Core::ELayer::Editor:
					if (std::shared_ptr<CLayer> Layer = LayerStack.GetLayer("Editor")) {
						LK_INFO_TAG("Application", "Removing layer: {}", Enum::ToString(LayerType));
						LayerStack.PopLayer(Layer);
						CRenderer::SetClearColor(FColor::Convert(RGBA32::DarkerGray));
					}
					break;
				default:
					LK_VERIFY(false);
					break;
			}

			Core::Global.LayerRemoveQueue.pop();
		}
	}

}
