#pragma once

#include <filesystem>

#include "core/core.h"
#include "core/assert.h"
#include "core/buffer.h"
#include "opengl.h"
#include "texture_enums.h"

namespace platformer2d {

	class CTexture;

	struct FTextureArraySpecification
	{
		EImageFormat ImageFormat = EImageFormat::RGBA;
		uint8_t Layers = 2;
		uint32_t Width = 1;
		uint32_t Height = 1;
		uint8_t Mips = 4;
		std::string DebugName{};
	};

	class CTextureArray
	{
	public:
		CTextureArray(const FTextureArraySpecification& Specification);
		CTextureArray() = delete;
		~CTextureArray();

		void Bind(uint32_t Slot = 0) const;
		void Unbind(uint32_t Slot = 0) const;

		bool AddTexture(std::shared_ptr<CTexture> Texture);

		LRendererID GetRendererID() const { return RendererID; }
		uint32_t GetWidth() const { return Width; }
		uint32_t GetHeight() const { return Height; }
		const std::filesystem::path& GetFilePath() const { return Path; }

	public:
		static constexpr int MAX_TEXTURES = 10;
	private:
		LRendererID RendererID;
		std::vector<std::shared_ptr<CTexture>> Textures{};

		uint32_t Width = 1;
		uint32_t Height = 1;
		uint32_t Layers = 0;
		std::filesystem::path Path{};
		std::string DebugName{};
	};

}