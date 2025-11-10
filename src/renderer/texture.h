#pragma once

#include <filesystem>

#include "core/core.h"
#include "core/assert.h"
#include "core/buffer.h"
#include "opengl.h"
#include "texture_enums.h"

namespace platformer2d {

	enum class ETexture
	{
		White,
		Background,
		Player,
		Metal,
		Bricks,
		Wood,
		Swoosh,
	};

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

		LRendererID GetID() const { return ID; }
		uint32_t GetWidth() const { return Width; }
		uint32_t GetHeight() const { return Height; }
		uint8_t GetChannels() const { return Channels; }
		uint8_t GetMips() const { return Mips; }
		const std::filesystem::path& GetFilePath() const { return Path; }

		void SetWrap(ETextureWrap InWrap) const;
		void SetFilter(ETextureFilter InFilter) const;

		std::size_t GetSlot() const { return Slot; }
		void SetSlot(std::size_t InSlot);

		[[nodiscard]] const FBuffer& GetImageBuffer() const { return ImageBuffer; }
		[[nodiscard]] const std::string& GetDebugName() const { return DebugName; }

	private:
		LRendererID ID{};
		FBuffer ImageBuffer;
		std::size_t Slot;

		uint32_t Width = 1;
		uint32_t Height = 1;
		uint8_t Channels = 0;
		uint8_t Mips = 1;
		std::filesystem::path Path{};
		std::string DebugName{};

		GLenum Format{};
		GLenum InternalFormat{};
		GLenum DataType{};

		static_assert(std::is_same_v<LRendererID, GLuint>, "LRendererID type mismatch");
	};

	namespace Enum
	{
		inline constexpr const char* ToString(const ETexture Texture)
		{
			switch (Texture)
			{
				case ETexture::White:      return "White";
				case ETexture::Background: return "Background";
				case ETexture::Player:     return "Player";
				case ETexture::Bricks:     return "Bricks";
				case ETexture::Metal:      return "Metal";
				case ETexture::Wood:       return "Wood";
				case ETexture::Swoosh:     return "Swoosh";
				default: break;
			}
			return nullptr;
		}
	}

}