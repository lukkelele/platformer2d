#include "texture.h"

#include <stb/stb_image.h>

#include "core/log.h"

namespace platformer2d {

	CTexture::CTexture()
	{
	}

	CTexture::CTexture(const FTextureSpecification& Specification)
		: Path(Specification.Path)
	{
		LK_ASSERT((Specification.Width > 0) && (Specification.Height > 0) && !Specification.Path.empty());
		LK_OpenGL_Verify(glCreateTextures(GL_TEXTURE_2D, 1, &RendererID));
		LK_OpenGL_Verify(glBindTexture(GL_TEXTURE_2D, RendererID));

		stbi_set_flip_vertically_on_load(Specification.bFlipVertical);
		int ReadWidth, ReadHeight, ReadChannels;
		float* Data = stbi_loadf(Specification.Path.c_str(), &ReadWidth, &ReadHeight, &ReadChannels, 4);
		LK_ASSERT(Data != NULL, "Failed to load texture from: {}", Specification.Path);
		if ((ReadWidth != Specification.Width) || (ReadHeight != Specification.Height))
		{
			LK_WARN("Texture mismatch ({}) between specified and actual size ({}x{} != {}x{})",
					Path.filename().generic_string(), Specification.Width, Specification.Height,
					ReadWidth, ReadHeight);
		}

		Width = ReadWidth;
		Height = ReadHeight;
		const uint64_t ImageSize = OpenGL::CalculateImageSize(Specification.Format, Width, Height);
		LK_DEBUG("Image size: {} bytes (Channels: {})", ImageSize, Channels);
		ImageData = FBuffer(Data, ImageSize);

		Format = OpenGL::GetImageFormat(Specification.Format);
		InternalFormat = OpenGL::GetImageInternalFormat(Specification.Format);
		DataType = OpenGL::GetFormatDataType(Specification.Format);
		LK_TRACE_TAG("Texture", "Format: {} ({})", Enum::ToString(Specification.Format), Format);
		LK_TRACE_TAG("Texture", "Internal Format: {}", InternalFormat);

		if (Data)
		{
			LK_OpenGL_Verify(glTexImage2D(
				GL_TEXTURE_2D,
				0,
				InternalFormat,
				ReadWidth,
				ReadHeight,
				0,
				Format,
				DataType,
				Data
			));

			stbi_image_free(Data);
		}

		const bool bMipmap = (Specification.Mips > 1);
		if (bMipmap)
		{
			glGenerateMipmap(RendererID);
		}

		OpenGL::SetTextureWrap(Specification.SamplerWrap);
		OpenGL::SetTextureFilter(Specification.SamplerFilter, bMipmap);
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
			LK_OpenGL_Verify(glTexImage2D(
				GL_TEXTURE_2D,
				0,
				InternalImageFormat,
				Width,
				Height,
				0,
				ImageFormat,
				GL_FLOAT,
				(const void*)InData
			));

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

	void CTexture::Invalidate()
	{
		if (RendererID)
		{
			LK_OpenGL_Verify(glDeleteTextures(1, &RendererID));
			RendererID = 0;
		}

		const uint32_t MipCount = OpenGL::CalculateMipCount(Width, Height);
		LK_OpenGL_Verify(glCreateTextures(GL_TEXTURE_2D, 1, &RendererID));
		LK_OpenGL_Verify(glTextureStorage2D(
			RendererID,
			MipCount,
			InternalFormat,
			Width,
			Height
		));

		if (ImageData)
		{
			LK_OpenGL_Verify(glTextureSubImage2D(
				RendererID,
				0,
				0,
				0,
				Width,
				Height,
				Format,
				DataType,
				ImageData.Data
			));
		}
	}

}
