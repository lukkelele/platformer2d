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
		bool bFlipVertical = true;

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
		CTexture(uint32_t InWidth, uint32_t InHeight, void* InData = nullptr);
		~CTexture() = default;

		void Bind(uint32_t Slot = 0) const;
		void Unbind(uint32_t Slot = 0) const;

		void Invalidate();

		LRendererID GetRendererID() const { return RendererID; }
		uint32_t GetWidth() const { return Width; }
		uint32_t GetHeight() const { return Height; }
		uint32_t GetChannels() const { return Channels; }

		std::size_t GetIndex() const { return Index; }
		void SetIndex(std::size_t InIndex);

	private:
		LRendererID RendererID{};
		FBuffer ImageData;
		std::size_t Index;

		uint32_t Width = 1;
		uint32_t Height =1;
		uint8_t Channels;
		std::filesystem::path Path{};

		GLenum Format{};
		GLenum InternalFormat{};
		GLenum DataType{};
		
		static_assert(std::is_same_v<LRendererID, GLuint>, "LRendererID type mismatch");
	};

}