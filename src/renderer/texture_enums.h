#pragma once

#include "core/assert.h"
#include "core/enum.h"

namespace platformer2d {

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

		COUNT,
		Depth = DEPTH24STENCIL8, /* Alias */
	};
	LK_ENUM(EImageFormat);

	enum class ETextureWrap
	{
		Clamp,
		Repeat,
		COUNT
	};
	LK_ENUM(ETextureWrap);

	enum class ETextureFilter
	{
		Nearest,
		Linear,
		COUNT
	};
	LK_ENUM(ETextureFilter);

	enum class ETextureUniformType : std::uint8_t
	{
		Diffuse = 0,
		Specular,
		Normal,
		Height,
		Emissive,
		DiffuseRoughness,
		COUNT
	};
	LK_ENUM(ETextureUniformType);

	struct FTextureSpecification
	{
		std::filesystem::path Path{};
		std::string Name{};
		std::uint32_t Width = 1;
		std::uint32_t Height = 1;
		std::uint8_t Mips = 1;
		bool bFlipVertical = true;
		bool bInvert = false;

		EImageFormat Format = EImageFormat::RGBA;
		ETextureWrap SamplerWrap = ETextureWrap::Clamp;
		ETextureFilter SamplerFilter = ETextureFilter::Linear;

		ETextureUniformType UniformType = ETextureUniformType::Diffuse;

		bool bStorage = false;
		bool bStoreLocally = false;
	};
}

