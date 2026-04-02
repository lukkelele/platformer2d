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
		LKLOG_TRACE_TAG("RenderThread", "Run");
		Thread.Run(EThreadPolicy::Detach);
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
	}

}
