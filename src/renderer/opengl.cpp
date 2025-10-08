#include "opengl.h"

#include <glm/glm.hpp>

namespace platformer2d::OpenGL {

	void LoadInfo(FBackendInfo& Info)
	{
		int Major, Minor;
		LK_OpenGL_Verify(glGetIntegerv(GL_MAJOR_VERSION, &Major));
		LK_OpenGL_Verify(glGetIntegerv(GL_MINOR_VERSION, &Minor));
		Info.Version.Major = Major;
		Info.Version.Minor = Minor;

		int ExtensionCount;
		glGetIntegerv(GL_NUM_EXTENSIONS, &ExtensionCount);
		Info.Extensions.reserve(ExtensionCount);
		for (int Idx = 0; Idx < ExtensionCount; Idx++)
		{
			Info.Extensions.push_back(std::string(reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, Idx))));
		}
	}

	void SetTextureFilter(const ETextureFilter TextureFilter, const bool IsMipmap)
	{
		switch (TextureFilter)
		{
			case ETextureFilter::Linear:
			{
				LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (IsMipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR)));
				LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
				break;
			}
			case ETextureFilter::Nearest:
			{
				LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (IsMipmap ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST)));
				LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
				break;
			}
		}
	}

	void SetTextureFilter(const LRendererID ID, const ETextureFilter TextureFilter, const bool IsMipmap)
	{
		switch (TextureFilter)
		{
			case ETextureFilter::Linear:
			{
				LK_OpenGL_Verify(glTextureParameteri(ID, GL_TEXTURE_MIN_FILTER, (IsMipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR)));
				LK_OpenGL_Verify(glTextureParameteri(ID, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
				break;
			}
			case ETextureFilter::Nearest:
			{
				LK_OpenGL_Verify(glTextureParameteri(ID, GL_TEXTURE_MIN_FILTER, (IsMipmap ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST)));
				LK_OpenGL_Verify(glTextureParameteri(ID, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
				break;
			}
		}
	}

	void SetTextureWrap(const ETextureWrap TextureWrap)
	{
		switch (TextureWrap)
		{
			case ETextureWrap::Clamp:
			{
				LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
				LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
				break;
			}
			case ETextureWrap::Repeat:
			{
				LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
				LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
				break;
			}
		}
	}

	void SetTextureWrap(const LRendererID ID, const ETextureWrap TextureWrap)
	{
		switch (TextureWrap)
		{
			case ETextureWrap::Clamp:
			{
				LK_OpenGL_Verify(glTextureParameteri(ID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
				LK_OpenGL_Verify(glTextureParameteri(ID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
				break;
			}
			case ETextureWrap::Repeat:
			{
				LK_OpenGL_Verify(glTextureParameteri(ID, GL_TEXTURE_WRAP_S, GL_REPEAT));
				LK_OpenGL_Verify(glTextureParameteri(ID, GL_TEXTURE_WRAP_T, GL_REPEAT));
				break;
			}
			default: 
				LK_ASSERT(false, "Unknown texture wrap: {}", static_cast<int>(TextureWrap));
		}
	}

	GLenum GetImageFormat(const EImageFormat Format)
	{
		switch (Format)
		{
			case EImageFormat::RGB: return GL_RGB;

			/* RGBA. */
			case EImageFormat::RGBA:
			case EImageFormat::RGBA8:
			case EImageFormat::RGBA16F:
			case EImageFormat::RGBA32F: return GL_RGBA;

			/* SRGB. */
			case EImageFormat::SRGB:  return GL_SRGB;
			case EImageFormat::SRGBA: return GL_SRGB_ALPHA;
		}

		LK_ASSERT(false, "Unknown image format: {}", static_cast<int>(Format));
		return GL_INVALID_ENUM;
	}

	GLenum GetFormatDataType(const EImageFormat Format)
	{
		switch (Format)
		{
			case EImageFormat::RGB:
			case EImageFormat::RGBA:
			case EImageFormat::RGBA8:	return GL_UNSIGNED_BYTE;
			case EImageFormat::RGBA16F:
			case EImageFormat::RGBA32F: return GL_FLOAT;
		}

		LK_ASSERT(false, "Unknown image format: {}", static_cast<int>(Format));
		return GL_INVALID_VALUE;
	}

	GLenum GetImageInternalFormat(const EImageFormat Format)
	{
		switch (Format)
		{
#if 0
			case EImageFormat::RGB:				return GL_RGB8;
			case EImageFormat::RGBA:			return GL_RGBA8;
			case EImageFormat::RGBA8:			return GL_RGBA8;
			case EImageFormat::RGBA16F:			return GL_RGBA16F;
			case EImageFormat::RGBA32F:			return GL_RGBA32F;
			case EImageFormat::DEPTH24STENCIL8: return GL_DEPTH24_STENCIL8;
			case EImageFormat::DEPTH32F:		return GL_DEPTH_COMPONENT32F;

#else
			case EImageFormat::RGB:		return GL_RGB32F;
			case EImageFormat::RGB8:	return GL_RGB8;

			case EImageFormat::RGBA:	return GL_RGBA32F;
			case EImageFormat::RGBA8:	return GL_RGBA8;
			case EImageFormat::RGBA16F: return GL_RGBA16F;
			case EImageFormat::RGBA32F: return GL_RGBA32F;

			case EImageFormat::RG16F:	return GL_RG16F;
			case EImageFormat::RG32F:	return GL_RG32F;

			case EImageFormat::RED8UI:	return GL_R8UI;
			case EImageFormat::RED16UI: return GL_R16UI;
			case EImageFormat::RED32UI: return GL_R32UI;
			case EImageFormat::RED32F:	return GL_R32F;
#endif
		}

		LK_ASSERT(false, "Unknown image format: {}", static_cast<int>(Format));
		return GL_INVALID_ENUM;
	}

	GLenum GetSamplerWrap(const ETextureWrap TextureWrap)
	{
		switch (TextureWrap)
		{
			case ETextureWrap::Clamp:  return GL_CLAMP_TO_EDGE;
			case ETextureWrap::Repeat: return GL_REPEAT;
		}

		LK_ASSERT(false, "Unknown sampler wrap: {}", static_cast<int>(TextureWrap));
		return GL_INVALID_VALUE;
	}

	GLenum GetSamplerFilter(const ETextureFilter TextureFilter, const bool IsMipmap)
	{
		switch (TextureFilter)
		{
			case ETextureFilter::Linear:  return IsMipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
			case ETextureFilter::Nearest: return IsMipmap ? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST;
		}

		LK_ASSERT(false, "Unknown sampler filter: {} {}", static_cast<int>(TextureFilter), IsMipmap ? "(Mipmap)" : "");
		return GL_INVALID_VALUE;
	}

	GLenum ImageFormatToDataFormat(const EImageFormat Format)
	{
		switch (Format)
		{
			case EImageFormat::RGBA:
			case EImageFormat::RGBA8:
			case EImageFormat::RGBA16F:
			case EImageFormat::RGBA32F: return GL_RGBA;

			case EImageFormat::RG8:
			case EImageFormat::RG16F:
			case EImageFormat::RG32F:	return GL_RG;

			case EImageFormat::RGB:
			case EImageFormat::RGB8:	return GL_RGB;

			case EImageFormat::RED8UI:
			case EImageFormat::RED16UI:
			case EImageFormat::RED32UI: return GL_RED_INTEGER;
			case EImageFormat::RED32F:	return GL_RED_INTEGER;
		}

		LK_ASSERT(false, "Invalid image format: {}", static_cast<int>(Format));
		return GL_INVALID_VALUE;
	}

	uint32_t CalculateMipCount(const uint32_t Width, const uint32_t Height)
	{
		return static_cast<uint32_t>(std::floor(std::log2(glm::min(Width, Height))) + 1);
	}

	uint32_t GetFormatBPP(const EImageFormat ImageFormat)
	{
		switch (ImageFormat)
		{
			case EImageFormat::RGB:
			case EImageFormat::RGBA8:   return 4;
			case EImageFormat::RGBA:    return 4;
			case EImageFormat::RGBA16F: return 2 * 4;
			case EImageFormat::RGBA32F: return 4 * 4;
		}

		LK_ASSERT(false, "Invalid image format: {}", static_cast<int>(ImageFormat));
		return 0;
	}

	uint32_t CalculateImageSize(const EImageFormat ImageFormat, const uint32_t Width, const uint32_t Height)
	{
		return (Width * Height * GetFormatBPP(ImageFormat));
	}

}