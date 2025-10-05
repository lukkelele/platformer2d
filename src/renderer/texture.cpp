#include "texture.h"

#include <stb/stb_image.h>

#include "core/log.h"
#include "renderer/opengl.h"

namespace platformer2d {

	static_assert(std::is_same_v<LRendererID, GLuint>, "RendererID type mismatch");

	CTexture::CTexture()
	{
	}

	CTexture::CTexture(const uint16_t InWidth, const uint16_t InHeight, void* InData)
		: Width(InWidth)
		, Height(InHeight)
	{
		LK_ASSERT((InWidth > 0) && (InHeight > 0));
		LK_OpenGL_Verify(glGenTextures(1, &RendererID));
		LK_OpenGL_Verify(glBindTexture(GL_TEXTURE_2D, RendererID));

		/**
		 * @todo Pass texture wrap/filter args from constructor.
		 * Also pass other metadata to know if stbi_load/stbi_loadf was used
		 * to correctly map GL_FLOAT/GL_UNSIGNED_BYTE.
		 */
		static const GLenum ImageFormat = GL_RGBA;
		static const GLenum InternalImageFormat = GL_RGBA32F;

		if (InData)
		{
			LK_DEBUG("glTexImage2D");
			LK_OpenGL_Verify(glTexImage2D(
				GL_TEXTURE_2D, 
				0, 
				InternalImageFormat, 
				Width, 
				Height,
				0,
				ImageFormat, 
				GL_FLOAT, 
				(const void*)InData)
			);

			spdlog::warn("Release texture data");
			stbi_image_free(InData);
		}

		/* Wrap. */
		/* @fixme */
		LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		//LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
		//LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));

		/* Filter. */
		/* @fixme */
		LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		//LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		//LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	}

}
