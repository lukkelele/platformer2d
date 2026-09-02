#include "rendercommandqueue.h"

#include <cstring>
#include <memory>

#include "core/profiler.h"

namespace platformer2d {

	CRenderCommandQueue::CRenderCommandQueue()
	{
		CommandBuffer = new std::uint8_t[10 * 1024 * 1024];
		CommandBufferPtr = CommandBuffer;
		std::memset(CommandBuffer, 0, 10 * 1024 * 1024);
	}

	CRenderCommandQueue::~CRenderCommandQueue()
	{
		delete[] CommandBuffer;
	}

	void CRenderCommandQueue::Execute()
	{
		LK_PROFILER_SCOPED();
		/*
		  +----------------+-----------------+--------------+
		  | Render Command | Size (uint32_t) | Command data |
		  +----------------+-----------------+--------------+
		 */
		std::uint8_t* TargetBuf = CommandBuffer;
		for (std::uint32_t i = 0; i < CommandCount; i++) {
			FRenderCommand RenderCommand = *(FRenderCommand*)TargetBuf;
			TargetBuf += sizeof(FRenderCommand);

			const std::uint32_t Size = *(uint32_t*)TargetBuf;
			TargetBuf += sizeof(std::uint32_t);

			RenderCommand(TargetBuf);
			TargetBuf += Size;
		}

		CommandBufferPtr = CommandBuffer;
		CommandCount = 0;
	}

	void* CRenderCommandQueue::Allocate(FRenderCommand RenderCommand, const std::uint32_t Size)
	{
		/*
		  +----------------+-----------------+--------------+
		  | Render Command | Size (uint32_t) | Command data |
		  +----------------+-----------------+--------------+
		 */
		*(FRenderCommand*)CommandBufferPtr = RenderCommand;
		CommandBufferPtr += sizeof(FRenderCommand);

		*(std::uint32_t*)(CommandBufferPtr) = Size;
		CommandBufferPtr += sizeof(std::uint32_t);

		void* MemPtr = CommandBufferPtr;
		CommandBufferPtr += Size;
		CommandCount++;

		return MemPtr;
	}

}
