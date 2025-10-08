#pragma once

#include <filesystem>

#include "core/core.h"
#include "core/assert.h"
#include "core/buffer.h"
#include "opengl.h"
#include "texture_enums.h"

namespace platformer2d {

	struct FTextureSpecification
	{
		std::string Path{};
		std::string Name{};
		uint32_t Width = 1;
		uint32_t Height = 1;
		uint8_t Mips = 1;
		bool bFlipVertical = false;

		EImageFormat Format = EImageFormat::RGBA;
		ETextureWrap SamplerWrap = ETextureWrap::Clamp;
		ETextureFilter SamplerFilter = ETextureFilter::Linear;

		ETextureUniformType UniformType = ETextureUniformType::Diffuse;

		bool bStorage = false;
		bool bStoreLocally = false;
	};

	class CTexture
	{
	public:
		CTexture();
		CTexture(const FTextureSpecification& Specification);
		CTexture(uint16_t InWidth, uint16_t InHeight, void* InData = nullptr);
		~CTexture() = default;

		FORCEINLINE void Bind(const uint32_t Slot = 0) const
		{
			LK_OpenGL_Verify(glBindTextureUnit(Slot, RendererID));
		}

		FORCEINLINE void Unbind(const uint32_t Slot = 0) const
		{
			LK_OpenGL_Verify(glBindTextureUnit(Slot, 0));
		}

		void Invalidate();

		LRendererID GetRendererID() const { return RendererID; }
		uint32_t GetWidth() const { return Width; }
		uint32_t GetHeight() const { return Height; }
		uint32_t GetChannels() const { return Channels; }

	private:
		LRendererID RendererID{};
		FBuffer ImageData;

		uint32_t Width{};
		uint32_t Height{};
		uint8_t Channels{};
		std::filesystem::path Path{};

		GLenum Format{};
		GLenum InternalFormat{};
		GLenum DataType{};
		
		static_assert(std::is_same_v<LRendererID, GLuint>, "LRendererID type mismatch");
	};

}