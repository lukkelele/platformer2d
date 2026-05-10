#pragma once

#include "core/window.h"
#include "core/layerstack.h"
#include "core/settings.h"
#include "core/timer.h"
#include "renderer/renderer.h"
#include "renderer/ui/uilayer.h"

namespace platformer2d {

	class CApplication
	{
	public:
		CApplication(int Argc, char* Argv[]);
		CApplication() = delete;
		CApplication(const CApplication&) = delete;
		CApplication(CApplication&&) = delete;
		virtual ~CApplication();

		CApplication& operator=(const CApplication&) = delete;
		CApplication& operator=(CApplication&&) = delete;

		virtual void Initialize();
		virtual void Shutdown();
		virtual void Run();

		bool PushLayer(std::shared_ptr<CLayer> Layer);
		void RenderUI();

	private:
		void CheckLayerQueues();

	protected:
		bool bRunning = false;
		std::shared_ptr<CUILayer> UILayer;
		CLayerStack LayerStack;
		CTimer Timer;
	};

}

