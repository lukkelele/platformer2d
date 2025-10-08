#pragma once


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

}