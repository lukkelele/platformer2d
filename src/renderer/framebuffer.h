#pragma once

#include "core/core.h"
#include "core/enum.h"
#include "renderer/color.h"
#include "renderer/texture.h"

namespace platformer2d {

	enum class EFramebufferTextureFormat
	{
		None = 0,
		RGBA8,
		RED_INTEGER,
		DEPTH24STENCIL8, /* Depth/Stencil */
		COUNT,
		Depth = DEPTH24STENCIL8, /* Alias */
	};
	LK_ENUM(EFramebufferTextureFormat);

	struct FFramebufferTextureSpecification
	{
		FFramebufferTextureSpecification(const EImageFormat InImageFormat)
			: ImageFormat(InImageFormat)
		{}
		FFramebufferTextureSpecification() = default;

		EImageFormat ImageFormat{};
		bool bBlend = true;
	};

	struct FFramebufferAttachmentSpecification
	{
		FFramebufferAttachmentSpecification() = default;
		FFramebufferAttachmentSpecification(const std::initializer_list<FFramebufferTextureSpecification>& InAttachments)
			: Attachments(InAttachments)
		{}

		std::vector<FFramebufferTextureSpecification> Attachments{};
	};

	struct FFramebufferSpecification
	{
		float Scale = 1.0f;
		uint32_t Width = 0;
		uint32_t Height = 0;
		glm::vec4 ClearColor = {0.0f, 0.0f, 0.0f, 1.0f};
		float DepthClearValue = 0.0f;
		bool ClearColorOnLoad = true;
		bool ClearDepthOnLoad = true;
		uint32_t Samples = 1;

		FFramebufferAttachmentSpecification Attachments{};

		bool SwapChainTarget = false;
		bool Transfer = false;
		bool Blend = true;

		std::string Name;
	};

	class CFramebuffer
	{
	public:
		CFramebuffer(const FFramebufferSpecification& InSpec);
		CFramebuffer() = delete;
		~CFramebuffer();

		void Destroy();
		void Invalidate();

		void BindTexture(std::uint32_t AttachmentIdx, std::uint32_t Slot) const;
		void Bind() const;
		void Unbind() const;
		void TargetSwapChain() const { Unbind(); }

		[[nodiscard]] LRendererID GetID() const { return ID; }
		[[nodiscard]] std::uint32_t GetWidth() const { return Spec.Width; }
		[[nodiscard]] std::uint32_t GetHeight() const { return Spec.Height; }
		[[nodiscard]] std::uint64_t GetSize() const { return (Spec.Width * Spec.Height); }

		/** @brief Clear the default framebuffer. */
		static void ClearDefault();
		static void SetDefaultClearColor(const glm::vec4& InClearColor) { DefaultClearColor = InClearColor; }

		void Clear() const;
		void ClearAttachment(std::uint32_t AttachmentIdx, int Value);
		[[nodiscard]] std::uint32_t GetColorAttachmentID(uint32_t Idx = 0) const;

		[[nodiscard]] std::shared_ptr<CTexture> GetImage(uint32_t AttachmentIdx = 0);
		void Resize(std::uint32_t InWidth, std::uint32_t InHeight);
		[[nodiscard]] int ReadPixel(std::uint32_t AttachmentIdx, int PosX, int PosY);

		void SetClearColor(const glm::vec4& InClearColor) { ClearColor = InClearColor; }
		[[nodiscard]] const glm::vec4& GetClearColor() const { return ClearColor; }

	private:
		LRendererID ID = 0;
		glm::vec4 ClearColor;
		static inline glm::vec4 DefaultClearColor = FColor::Convert(RGBA32::DarkerGray); /* FB0 */

		std::vector<std::shared_ptr<CTexture>> ColorAttachments{};
		std::uint32_t DepthAttachment = 0;

		FFramebufferSpecification Spec;
		FFramebufferTextureSpecification DepthAttachmentSpec{};
		std::vector<FFramebufferTextureSpecification> ColorAttachmentSpecs{};
	};
}

