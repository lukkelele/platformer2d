#pragma once

#include <filesystem>

#include "core/core.h"
#include "core/enum.h"
#include "core/assert.h"
#include "core/buffer.h"
#include "opengl.h"
#include "texture_enums.h"

namespace platformer2d {

	enum class ETexture : std::uint8_t
	{
		White,
		Player,
		Metal,
		Bricks,
		Wood,
		Swoosh,
		Cloud,
		Rifle,
		Axe,
		Goblin,
		GoblinArmored,
		COUNT
	};
	LK_ENUM(ETexture);

	class CTexture
	{
	public:
		CTexture(const FTextureSpecification& Specification);
		CTexture(const FTextureSpecification& Specification, const FBuffer& InData);
		CTexture(std::uint32_t InWidth, std::uint32_t InHeight, void* InData = nullptr);
		CTexture() = delete;
		CTexture(CTexture&&) = delete;
		CTexture(const CTexture&) = default;
		~CTexture();

		CTexture& operator=(CTexture&&) = delete;
		CTexture& operator=(const CTexture&) = delete;

		void Bind(std::uint32_t Slot = 0) const;
		void Unbind(std::uint32_t Slot = 0) const;

		void Invalidate();

		bool Reload(const FTextureSpecification& NewSpec);

		[[nodiscard]] LRendererID GetID() const { return ID; }
		[[nodiscard]] std::uint32_t GetWidth() const { return Width; }
		[[nodiscard]] std::uint32_t GetHeight() const { return Height; }
		[[nodiscard]] std::uint8_t GetChannels() const { return Channels; }
		[[nodiscard]] std::uint8_t GetMips() const { return Mips; }
		[[nodiscard]] const std::filesystem::path& GetFilePath() const { return Path; }

		void SetWrap(ETextureWrap InWrap) const;
		void SetFilter(ETextureFilter InFilter) const;

		[[nodiscard]] std::size_t GetSlot() const { return Slot; }
		void SetSlot(std::size_t InSlot);

		[[nodiscard]] const FBuffer& GetImageBuffer() const { return ImageBuffer; }
		[[nodiscard]] const std::string& GetName() const { return Name; }

	private:
		LRendererID ID{};
		FBuffer ImageBuffer;
		std::size_t Slot;

		std::uint32_t Width = 1;
		std::uint32_t Height = 1;
		std::uint8_t Channels = 0;
		std::uint8_t Mips = 1;
		std::filesystem::path Path{};
		std::string Name{};

		GLenum DataFormat{}; /* Format of pixel data. */
		GLenum InternalFormat{};
		GLenum DataType{};

		static_assert(std::is_same_v<LRendererID, GLuint>, "LRendererID type mismatch");
	};
}
