#include "renderthread.h"

#include "renderer/renderer.h"

namespace platformer2d {

	void CRenderThread::Initialize()
	{
		LKLOG_TRACE_TAG("RenderThread", "Initialize");
		Thread.Setup(CRenderer::Thread, std::ref(*this));
	}

	void CRenderThread::Run()
	{
		Thread.Run(EThreadPolicy::Detach);
		LKLOG_DEBUG_TAG("RenderThread", "Thread started");
	}

	void CRenderThread::Terminate()
	{
		LKLOG_DEBUG_TAG("RenderThread", "Destroy");
		bShouldTerminate = true;
	}

	bool CRenderThread::IsRunning() const
	{
		return Thread.IsRunning();
	}

	bool CRenderThread::ShouldTerminate() const
	{
		return bShouldTerminate.load();
	}

	void CRenderThread::NextFrame()
	{
		CRenderer::SwapQueues();
	}

	void CRenderThread::WaitFor(const EThreadState WaitForState)
	{
		std::unique_lock Lock(Mutex);
		CondVar.wait(Lock, [this, WaitForState]
		{
			return (State == WaitForState);
		});
	}

	void CRenderThread::Set(const EThreadState NewState)
	{
		std::unique_lock Lock(Mutex);
		State = NewState;
		Lock.unlock();

		CondVar.notify_all();
	}

	void CRenderThread::BlockUntilFinished()
	{
		WaitFor(EThreadState::Idle);
	}

	void CRenderThread::WakeUp()
	{
		Set(EThreadState::WakeUp);
	}

	void CRenderThread::Pump()
	{
		NextFrame();
		WakeUp();
		BlockUntilFinished();
	}

}
