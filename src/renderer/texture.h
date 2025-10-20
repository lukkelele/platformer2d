#pragma once

#include <filesystem>

#include "core/core.h"
#include "core/assert.h"
#include "core/buffer.h"
#include "opengl.h"
#include "texture_enums.h"

namespace platformer2d {

	class CTexture
	{
	public:
		CTexture(const FTextureSpecification& Specification);
		CTexture(uint32_t InWidth, uint32_t InHeight, void* InData = nullptr);
		CTexture() = delete;
		~CTexture() = default;

		void Bind(uint32_t Slot = 0) const;
		void Unbind(uint32_t Slot = 0) const;

		void Invalidate();

		LRendererID GetRendererID() const { return RendererID; }
		uint32_t GetWidth() const { return Width; }
		uint32_t GetHeight() const { return Height; }
		uint32_t GetChannels() const { return Channels; }
		const std::filesystem::path& GetFilePath() const { return Path; }

		std::size_t GetIndex() const { return Index; }
		void SetIndex(std::size_t InIndex);

		const FBuffer& GetImageBuffer() const { return ImageBuffer; }
		const std::string& GetDebugName() const { return DebugName; }

	private:
		LRendererID RendererID{};
		FBuffer ImageBuffer;
		std::size_t Index;

		uint32_t Width = 1;
		uint32_t Height = 1;
		uint8_t Channels = 0;
		std::filesystem::path Path{};
		std::string DebugName{};

		GLenum Format{};
		GLenum InternalFormat{};
		GLenum DataType{};

		static_assert(std::is_same_v<LRendererID, GLuint>, "LRendererID type mismatch");
	};

}