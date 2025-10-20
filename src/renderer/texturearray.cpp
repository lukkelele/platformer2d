#include "texturearray.h"

#include <stb/stb_image.h>

#include "texture.h"

namespace platformer2d {

	CTextureArray::CTextureArray(const FTextureArraySpecification& Specification)
		: Width(Specification.Width)
		, Height(Specification.Height)
		, Layers(Specification.Layers)
	{
		LK_ASSERT((Specification.Width > 0) && (Specification.Height > 0));
		LK_OpenGL_Verify(glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &RendererID));
		const GLuint InternalFormatGL = OpenGL::GetImageInternalFormat(Specification.ImageFormat);
		LK_OpenGL_Verify(glTextureStorage3D(RendererID, 1, InternalFormatGL, Width, Height, Layers));

		if (Specification.Mips > 1)
		{
			LK_OpenGL_Verify(glGenerateTextureMipmap(RendererID));
			LK_OpenGL_Verify(glTextureParameteri(RendererID, GL_TEXTURE_MAX_LEVEL, 10));
		}

		LK_OpenGL_Verify(glTextureParameteri(RendererID, GL_TEXTURE_MIN_FILTER, GL_NEAREST)); 
		LK_OpenGL_Verify(glTextureParameteri(RendererID, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
		LK_OpenGL_Verify(glTextureParameteri(RendererID, GL_TEXTURE_WRAP_S, GL_REPEAT));
		LK_OpenGL_Verify(glTextureParameteri(RendererID, GL_TEXTURE_WRAP_T, GL_REPEAT));
	}

	CTextureArray::~CTextureArray()
	{
		if (RendererID)
		{
			LK_TRACE_TAG("TextureArray", "Releasing resources (ID {})", RendererID);
			LK_OpenGL_Verify(glDeleteTextures(1, &RendererID));
		}
	}

	void CTextureArray::Bind(const uint32_t Slot) const
	{
		LK_OpenGL_Verify(glBindTextureUnit(Slot, RendererID));
	}

	void CTextureArray::Unbind(const uint32_t Slot) const
	{
		LK_OpenGL_Verify(glBindTextureUnit(Slot, 0));
	}

	bool CTextureArray::AddTexture(const std::shared_ptr<CTexture> Texture)
	{
		LK_ASSERT(Texture);
		if (!Texture)
		{
			return false;
		}
		if (Textures.size() >= MAX_TEXTURES)
		{
			LK_ERROR_TAG("TextureArray", "Reached max capacity: {}", Textures.size());
			return false;
		}

		/* Assume 2D texture. */
		FBuffer ImageBuffer = Texture->GetImageBuffer();
		LK_VERIFY(ImageBuffer.Data, "Missing image data");

		constexpr GLint XOffset = 0;
		constexpr GLint YOffset = 0;
		const GLint ZOffset = static_cast<GLint>(Textures.size());
		constexpr int Depth = 1;
		constexpr GLenum Format = GL_RGBA;
		constexpr GLenum DataType = GL_UNSIGNED_BYTE;

		LK_OpenGL_Verify(glTextureSubImage3D(
			RendererID,
			0, /* Level */
			XOffset, /* X Offset */
			YOffset, /* Y Offset */
			ZOffset, /* Z Offset */
			Width,
			Height,
			Depth,
			Format, /* Format */
			DataType, /* Type */
			ImageBuffer.Data
		));

		LK_DEBUG_TAG("TextureArray", "Add: {} (index {})", Texture->GetFilePath().filename(), Textures.size());
		Textures.push_back(Texture);

		return true;
	}

}