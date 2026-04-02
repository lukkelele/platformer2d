#pragma once

#include "core/thread.h"

namespace platformer2d {

	class CRenderThread
	{
	public:
		CRenderThread() = default;
		~CRenderThread() = default;
		CRenderThread(CRenderThread&&) = delete;
		CRenderThread(const CRenderThread&) = delete;
		CRenderThread& operator=(CRenderThread&&) = delete;
		CRenderThread& operator=(const CRenderThread&) = delete;

		void Initialize();
		void Run();
		void Terminate();
		bool IsRunning() const;
		bool ShouldTerminate() const;

		std::mutex& GetMutex() { return Mutex; }

	private:
		void NextFrame();

	private:
		CThread Thread;
		std::mutex Mutex;
		EThreadState State = EThreadState::None;
		std::atomic_bool bShouldTerminate = false;
	};

}
