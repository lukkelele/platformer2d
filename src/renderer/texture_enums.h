#pragma once

#include "core/assert.h"

namespace platformer2d {

	/**
	 * @enum EImageFormat
	 */
	enum class EImageFormat
	{
		None = 0,
		RED8UN,
		RED8UI,
		RED16UI,
		RED32UI,
		RED32F,
		RG8,
		RG16F,
		RG32F,
		RGB,
		RGBA,

		RGB8,
		RGBA8,

		RGBA16F,
		RGBA32F,

		B10R11G11UF,

		SRGB,
		SRGBA,

		DEPTH32FSTENCIL8UINT,
		DEPTH32F,
		DEPTH24STENCIL8,

		Depth = DEPTH24STENCIL8,
	};

	/**
	 * @enum ETextureWrap
	 */
	enum class ETextureWrap
	{
		Clamp,
		Repeat
	};

	/**
	 * @enum ETextureFilter
	 */
	enum class ETextureFilter
	{
		Nearest,
		Linear
	};

	/**
	 * @enum ETextureUniformType
	 */
	enum class ETextureUniformType : uint8_t
	{
		Diffuse = 0,
		Specular,
		Normal,
		Height,
		Emissive,
		DiffuseRoughness,
	};

	namespace Enum
	{
		static std::string ToString(EImageFormat Format)
		{
			switch (Format)
			{
				case EImageFormat::RG8:     return "RG8";
				case EImageFormat::RG16F:   return "RG16F";
				case EImageFormat::RG32F:   return "RG32F";
				case EImageFormat::RGB:     return "RGB";
				case EImageFormat::RGBA:    return "RGBA";
				case EImageFormat::RGBA8:   return "RGBA8";
				case EImageFormat::RGBA16F: return "RGBA16F";
				case EImageFormat::RGBA32F: return "RGBA32F";
				case EImageFormat::SRGB:    return "SRGB";
			}

			LK_ASSERT(false, "Unknown image format: {}", static_cast<int>(Format));
			return "Unknown";
		}
	}

}