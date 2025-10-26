#pragma once

#include "core/window.h"
#include "core/layerstack.h"
#include "core/timer.h"
#include "renderer/renderer.h"

namespace platformer2d {

	class CApplication
	{
	public:
		CApplication(int Argc, char* Argv[]);
		CApplication() = delete;
		virtual ~CApplication();

		virtual void Initialize();
		virtual void Shutdown();

		virtual void Run();

		bool PushLayer(std::shared_ptr<CLayer> Layer);

	protected:
		bool bRunning = false;
		std::unique_ptr<CWindow> Window;
		CLayerStack LayerStack;
		CTimer Timer;
	};

}
