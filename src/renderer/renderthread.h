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

		void NextFrame();
		void WaitFor(EThreadState WaitForState);
		void Set(EThreadState NewState);
		void BlockUntilFinished();
		void WakeUp();
		void Pump();

		std::mutex& GetMutex() { return Mutex; }

	private:
		CThread Thread;
		std::mutex Mutex;
		std::condition_variable CondVar;
		EThreadState State = EThreadState::Idle;
		std::atomic_bool bShouldTerminate = false;
	};

}
