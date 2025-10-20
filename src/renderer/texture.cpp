#include "texture.h"

#include <stb/stb_image.h>

#include "core/log.h"
#include "texturearray.h"

namespace platformer2d {

	namespace
	{
		std::size_t CreatedTextures = 0;
	}

	CTexture::CTexture(const FTextureSpecification& Specification)
		: Path(Specification.Path)
		, DebugName(Specification.DebugName)
	{
		LK_ASSERT((Specification.Width > 0) && (Specification.Height > 0) && !Specification.Path.empty());
		LK_OpenGL_Verify(glCreateTextures(GL_TEXTURE_2D, 1, &RendererID));
		LK_OpenGL_Verify(glBindTexture(GL_TEXTURE_2D, RendererID));

		Format = OpenGL::GetImageFormat(Specification.Format);
		InternalFormat = OpenGL::GetImageInternalFormat(Specification.Format);
		DataType = OpenGL::GetFormatDataType(Specification.Format);
		LK_TRACE_TAG("Texture", "Format: {} ({})", Enum::ToString(Specification.Format), Format);
		LK_TRACE_TAG("Texture", "Internal Format: {}", InternalFormat);

		stbi_set_flip_vertically_on_load(Specification.bFlipVertical);
		int ReadWidth, ReadHeight, ReadChannels;

		void* Data = nullptr;
		if (stbi_is_hdr(Specification.Path.c_str()))
		{
			LK_DEBUG_TAG("Texture", "[{}] HDR texture", Path.filename());
			Data = stbi_loadf(Specification.Path.c_str(), &ReadWidth, &ReadHeight, &ReadChannels, 4);
		}
		else
		{
			Data = stbi_load(Specification.Path.c_str(), &ReadWidth, &ReadHeight, &ReadChannels, 4);
		}
		LK_ASSERT(Data != NULL, "Failed to load texture from: {}", Specification.Path);
		if ((ReadWidth != Specification.Width) || (ReadHeight != Specification.Height))
		{
			LK_TRACE("Texture mismatch ({}) between specified and actual size ({}x{} != {}x{})",
					 Path.filename().generic_string(), Specification.Width, Specification.Height,
					 ReadWidth, ReadHeight);
		}

		Width = ReadWidth;
		Height = ReadHeight;
		Channels = ReadChannels;
		const uint64_t ImageSize = OpenGL::CalculateImageSize(Specification.Format, Width, Height);
		LK_ASSERT(ImageSize <= UINT64_MAX, "ImageSize overflow");
		LK_DEBUG("Image size: {} bytes (Channels: {})", ImageSize, Channels);
		ImageBuffer = FBuffer(Data, ImageSize);

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
			LK_ASSERT(ImageBuffer.Data);
		}

		const bool bMipmap = (Specification.Mips > 1);
		if (bMipmap)
		{
			LK_DEBUG_TAG("Texture", "[{}] Generating mipmap (Mips: {})", Path.filename(), Specification.Mips);
			LK_OpenGL_Verify(glGenerateTextureMipmap(RendererID));
		}
		else
		{
			LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0));
		}

		OpenGL::SetTextureWrap(Specification.SamplerWrap);
		OpenGL::SetTextureFilter(Specification.SamplerFilter, bMipmap);

		if (DebugName.empty())
		{
			DebugName = std::format("{}", Path.filename());
		}
		Index = CreatedTextures++;
		LK_DEBUG_TAG("Texture", "Index: {} ({})", Index, Path.filename());
	}

	CTexture::CTexture(const uint32_t InWidth, const uint32_t InHeight, void* InData)
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
		Channels = 4;

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

		LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

		DebugName = std::format("{}", Path.filename());
		Index = CreatedTextures++;
		LK_DEBUG_TAG("Texture", "Index: {}", Index);
	}

	void CTexture::Bind(const uint32_t Slot) const
	{
		LK_OpenGL_Verify(glBindTextureUnit(Slot, RendererID));
	}

	void CTexture::Unbind(const uint32_t Slot) const
	{
		LK_OpenGL_Verify(glBindTextureUnit(Slot, 0));
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

		if (ImageBuffer)
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
				ImageBuffer.Data
			));
		}
	}

	void CTexture::SetIndex(const std::size_t InIndex)
	{
		Index = InIndex;
	}

}
