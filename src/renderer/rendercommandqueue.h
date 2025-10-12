#pragma once

#include <cstdint>

namespace platformer2d {

	class CRenderCommandQueue
	{
	public:
		using FRenderCommand = void(*)(void*);

		CRenderCommandQueue();
		~CRenderCommandQueue();

		void Execute();
		void* Allocate(FRenderCommand RenderCommand, uint32_t Size);

	private:
		uint8_t* CommandBuffer = nullptr;
		uint8_t* CommandBufferPtr = nullptr;
		uint32_t CommandCount = 0;
	};

}