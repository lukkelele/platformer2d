#include "rendercommandqueue.h"

#include <cstring>
#include <memory>

namespace platformer2d {

	CRenderCommandQueue::CRenderCommandQueue()
	{
		CommandBuffer = new uint8_t[10 * 1024 * 1024];
		CommandBufferPtr = CommandBuffer;
		std::memset(CommandBuffer, 0, 10 * 1024 * 1024);
	}

	CRenderCommandQueue::~CRenderCommandQueue()
	{
		delete[] CommandBuffer;
	}

	void CRenderCommandQueue::Execute()
	{
		/*
		  +----------------+-----------------+--------------+
		  | Render Command | Size (uint32_t) | Command data | 
		  +----------------+-----------------+--------------+
		 */
		uint8_t* TargetBuf = CommandBuffer;
		for (uint32_t i = 0; i < CommandCount; i++)
		{
			FRenderCommand RenderCommand = *(FRenderCommand*)TargetBuf;
			TargetBuf += sizeof(FRenderCommand);

			const uint32_t Size = *(uint32_t*)TargetBuf;
			TargetBuf += sizeof(uint32_t);

			RenderCommand(TargetBuf);
			TargetBuf += Size;
		}

		CommandBufferPtr = TargetBuf;
		CommandCount = 0;
	}

	void* CRenderCommandQueue::Allocate(FRenderCommand RenderCommand, const uint32_t Size)
	{
		/*
		  +----------------+-----------------+--------------+
		  | Render Command | Size (uint32_t) | Command data | 
		  +----------------+-----------------+--------------+
		 */
		//*reinterpret_cast<FRenderCommand*>(CommandBufferPtr) = RenderCommand;
		*(FRenderCommand*)CommandBufferPtr = RenderCommand;
		CommandBufferPtr += sizeof(FRenderCommand);

		//*reinterpret_cast<uint32_t*>(CommandBufferPtr) = Size;
		*(uint32_t*)(CommandBufferPtr) = Size;
		CommandBufferPtr += sizeof(uint32_t);

		void* MemPtr = CommandBufferPtr;
		CommandBufferPtr += Size;
		CommandCount++;

		return MemPtr;
	}

}