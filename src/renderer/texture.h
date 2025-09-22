#pragma once

#include <stdint.h>

namespace platformer2d {

	class CTexture
	{
	public:
		CTexture();
		CTexture(uint16_t InWidth, uint16_t InHeight, void* InData = nullptr);
		~CTexture() = default;

	private:
		uint32_t RendererID{};
		uint16_t Width{};
		uint16_t Height{};
	};

}