#pragma once

#include "core/core.h"
#include "renderer/texture.h"

namespace platformer2d {

	enum class EFramebufferTextureFormat
	{
		None = 0,
		RGBA8,
		RED_INTEGER,
		DEPTH24STENCIL8, /* Depth/Stencil */
		Depth = DEPTH24STENCIL8
	};

	struct FFramebufferTextureSpecification
	{
		FFramebufferTextureSpecification(const EImageFormat InImageFormat)
			: ImageFormat(InImageFormat) {}
		FFramebufferTextureSpecification() = default;

		EImageFormat ImageFormat{};
		bool bBlend = true;
	};

	struct FFramebufferAttachmentSpecification
	{
		FFramebufferAttachmentSpecification() = default;
		FFramebufferAttachmentSpecification(const std::initializer_list<FFramebufferTextureSpecification>& InAttachments)
			: Attachments(InAttachments) {}

		std::vector<FFramebufferTextureSpecification> Attachments{};
	};

	struct FFramebufferSpecification
	{
		float Scale = 1.0f;
		uint32_t Width = 0;
		uint32_t Height = 0;
		glm::vec4 ClearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
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

		void BindTexture(uint32_t AttachmentIdx, uint32_t Slot) const;
		void Bind() const;
		void Unbind() const;
		FORCEINLINE void TargetSwapChain() const { Unbind(); }

		LRendererID GetID() const { return ID; }
		uint32_t GetWidth() const { return Spec.Width; }
		uint32_t GetHeight() const { return Spec.Height; }
		uint64_t GetSize() const { return (Spec.Width * Spec.Height); }

		void Clear() const;
		void ClearAttachment(uint32_t AttachmentIdx, int Value);
		uint32_t GetColorAttachmentID(uint32_t Idx = 0) const;

		std::shared_ptr<CTexture> GetImage(uint32_t AttachmentIdx = 0);
		void Resize(uint32_t InWidth, uint32_t InHeight);
		int ReadPixel(uint32_t AttachmentIdx, int PosX, int PosY);

	private:
		LRendererID ID = 0;
		glm::vec4 ClearColor;

		std::vector<std::shared_ptr<CTexture>> ColorAttachments{};
		uint32_t DepthAttachment = 0;

		FFramebufferSpecification Spec;
		FFramebufferTextureSpecification DepthAttachmentSpec{};
		std::vector<FFramebufferTextureSpecification> ColorAttachmentSpecs{};
	};

	namespace Enum
	{
		inline const char* ToString(const EFramebufferTextureFormat Format)
		{
			const char* S = "";
		#define _(EnumValue) case EFramebufferTextureFormat::EnumValue: S = #EnumValue; break
			switch (Format)
			{
				_(None);
				_(RGBA8);
				_(RED_INTEGER);
				_(DEPTH24STENCIL8);
				default:
					LK_THROW_ENUM_ERR(Format);
					break;
			}
		#undef _
			return S;
		}
	}


}
