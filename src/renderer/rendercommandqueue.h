#pragma once

#include <cstddef>
#include <cstdint>

namespace platformer2d {

	class CRenderCommandQueue
	{
	public:
		using FRenderCommand = void (*)(void*);

		CRenderCommandQueue();
		~CRenderCommandQueue();

		void Execute();
		void* Allocate(FRenderCommand RenderCommand, std::uint32_t Size);

	private:
		std::uint8_t* CommandBuffer = nullptr;
		std::uint8_t* CommandBufferPtr = nullptr;
		std::uint32_t CommandCount = 0;
	};

}
