#pragma once

#include "core/assert.h"
#include "core/core.h"
#include "opengl.h"

namespace platformer2d {

	class CTexture
	{
	public:
		CTexture();
		CTexture(uint16_t InWidth, uint16_t InHeight, void* InData = nullptr);
		~CTexture() = default;

		FORCEINLINE void Bind(const uint32_t Slot = 0) const
		{
			LK_OpenGL_Verify(glActiveTexture(GL_TEXTURE0 + Slot));
			LK_OpenGL_Verify(glBindTextureUnit(Slot, RendererID));
		}

		FORCEINLINE void Unbind(const uint32_t Slot = 0) const
		{
			/* LK_OpenGL_Verify(glActiveTexture(GL_TEXTURE0 + Slot)); */ /* @todo Needed or not? */
			LK_OpenGL_Verify(glBindTextureUnit(Slot, 0));
		}

		LRendererID GetRendererID() const { return RendererID; }

	private:
		LRendererID RendererID{};
		uint16_t Width{};
		uint16_t Height{};
	};

}