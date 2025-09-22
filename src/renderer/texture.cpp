#include "texture.h"

#include <type_traits>

#include <stb/stb_image.h>
#include <spdlog/spdlog.h>

#include "core/assert.h"
#include "renderer/opengl.h"

namespace platformer2d {

	CTexture::CTexture()
	{
		static_assert(std::is_same_v<decltype(RendererID), GLuint>, "RendererID type mismatch");
	}

	CTexture::CTexture(const uint16_t InWidth, const uint16_t InHeight, void* InData)
		: Width(InWidth)
		, Height(InHeight)
	{
		LK_ASSERT((InWidth > 0) && (InHeight > 0));

		LK_OpenGL_Verify(glGenTextures(1, &RendererID));
		LK_OpenGL_Verify(glBindTexture(GL_TEXTURE_2D, RendererID));
		spdlog::info("RendererID={}", RendererID);

		static const GLenum ImageFormat = GL_RGBA;
		static const GLenum InternalImageFormat = GL_RGBA32F;

		if (InData)
		{
			spdlog::info("glTexImage2D");
			LK_OpenGL_Verify(glTexImage2D(
				GL_TEXTURE_2D, 
				0, 
				InternalImageFormat, 
				Width, 
				Height,
				0,
				ImageFormat, 
				GL_UNSIGNED_BYTE, 
				(const void*)InData)
			);

			spdlog::warn("Release texture data");
			stbi_image_free(InData);
		}
	}

}
