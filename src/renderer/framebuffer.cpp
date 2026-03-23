#include "framebuffer.h"

#include "opengl.h"
#include "renderer.h"
#include "core/profiler.h"

namespace platformer2d {

	static bool IsDepthFormat(const EImageFormat ImageFormat);
	static GLenum GetTextureTarget(const bool Multisampled);
	static void AttachDepthTexture(const uint32_t ID, const int Samples, const GLenum Format, const GLenum AttachmentType, const uint32_t Width, const uint32_t Height);

	CFramebuffer::CFramebuffer(const FFramebufferSpecification& InSpec)
		: Spec(InSpec)
		, ClearColor(InSpec.ClearColor)
		, DepthAttachmentSpec(EImageFormat::DEPTH24STENCIL8)
	{
		LK_ASSERT((Spec.Width > 0) && (Spec.Height > 0), "Invalid framebuffer spec");
		for (FFramebufferTextureSpecification& FramebufferTextureSpec : Spec.Attachments.Attachments) {
			LK_TRACE_TAG("Framebuffer", "Iterating: {}", Enum::ToString(FramebufferTextureSpec.ImageFormat));
			if (!IsDepthFormat(FramebufferTextureSpec.ImageFormat)) {
				ColorAttachmentSpecs.emplace_back(FramebufferTextureSpec);
			} else {
				DepthAttachmentSpec = FramebufferTextureSpec;
			}
		}

		Invalidate();
		SetClearColor(ClearColor);
	}

	CFramebuffer::~CFramebuffer()
	{
		if (ID > 0) {
			Destroy();
		}
	}

	void CFramebuffer::Destroy()
	{
		LK_TRACE_TAG("Framebuffer", "Destroy: {}", ID);
		LK_OpenGL_Verify(glDeleteFramebuffers(1, &ID));
		ID = 0;

		ColorAttachments.clear();
		LK_OpenGL_Verify(glDeleteTextures(1, &DepthAttachment));
	}

	void CFramebuffer::Invalidate()
	{
		LK_TRACE_TAG("Framebuffer", "Invalidate");
		if (ID) {
			LK_TRACE_TAG("Framebuffer", "Delete existing framebuffer");
			LK_OpenGL_Verify(glDeleteFramebuffers(1, &ID));

			LK_TRACE_TAG("Framebuffer", "Clear color attachments");
			ColorAttachments.clear();

			LK_TRACE_TAG("Framebuffer", "Depth attachment: {}", DepthAttachment);
			if (DepthAttachment > 0) {
				LK_TRACE_TAG("Framebuffer", "Delete depth attachment");
				LK_OpenGL_Verify(glDeleteTextures(1, &DepthAttachment));
				DepthAttachment = 0;
			}
		}

		const bool Multisample = (Spec.Samples > 1);

		LK_OpenGL_Verify(glCreateFramebuffers(1, &ID));
		LK_OpenGL_Verify(glBindFramebuffer(GL_FRAMEBUFFER, ID));

		/* Color attachments. */
		LK_TRACE_TAG("Framebuffer", "Creating {} color attachments {} multisampling", ColorAttachmentSpecs.size(), Multisample ? "with" : "without");
		for (std::size_t Idx = 0; Idx < ColorAttachmentSpecs.size(); Idx++) {
			FTextureSpecification TexSpec;
			TexSpec.Width = Spec.Width;
			TexSpec.Height = Spec.Height;
			TexSpec.Format = ColorAttachmentSpecs[Idx].ImageFormat;
			TexSpec.Name = Format("fb-image-{}", Enum::ToString(ColorAttachmentSpecs[Idx].ImageFormat));
			TexSpec.Format = EImageFormat::RGBA32F;
			TexSpec.SamplerWrap = ETextureWrap::Clamp;
			TexSpec.SamplerFilter = ETextureFilter::Nearest;
			TexSpec.Mips = 1; /* No mipmapping. */
			TexSpec.bStorage = true;

			std::shared_ptr<CTexture> WhiteTexture = CRenderer::GetWhiteTexture();
			const std::filesystem::path TexturePath = WhiteTexture->GetFilePath();
			TexSpec.Path = TexturePath.string();
			LK_TRACE_TAG("Framebuffer", "{}: {}x{}", TexturePath.stem(), TexSpec.Width, TexSpec.Height);

			const FBuffer& ImageData = WhiteTexture->GetImageBuffer();
			LK_VERIFY(ImageData.Data, "Image data from white texture is NULL");
			std::shared_ptr<CTexture> Image = std::make_shared<CTexture>(TexSpec, ImageData);
			ColorAttachments.push_back(Image);

			LK_OpenGL_Verify(glFramebufferTexture2D(
				GL_FRAMEBUFFER,
				(GL_COLOR_ATTACHMENT0 + Idx),
				GL_TEXTURE_2D,
				ColorAttachments[Idx]->GetID(),
				0));
			LK_TRACE_TAG("Framebuffer", "ColorAttachment ID: {}  Size: ({}, {})", ColorAttachments[Idx]->GetID(), TexSpec.Width, TexSpec.Height);
		}

		/* Depth attachment. */
		if (DepthAttachmentSpec.ImageFormat != EImageFormat::None) {
			LK_TRACE_TAG("Framebuffer", "Attach depth texture");
			const GLenum TexTarget = GetTextureTarget(Multisample);
			LK_OpenGL_Verify(glCreateTextures(TexTarget, 1, &DepthAttachment));
			LK_OpenGL_Verify(glBindTexture(TexTarget, DepthAttachment));

			switch (DepthAttachmentSpec.ImageFormat) {
				case EImageFormat::DEPTH24STENCIL8:
					AttachDepthTexture(
						DepthAttachment,
						Spec.Samples,
						GL_DEPTH24_STENCIL8,
						GL_DEPTH_STENCIL_ATTACHMENT,
						Spec.Width,
						Spec.Height);
					break;
			}

			LK_TRACE_TAG("Framebuffer", "Created depth texture with an image format of DEPTH24STENCIL8");
		}

		LK_VERIFY(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is incomplete");
		LK_TRACE_TAG("Framebuffer", "Created framebuffer with {} color attachments", ColorAttachments.size());

		LK_OpenGL_Verify(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	}

	void CFramebuffer::BindTexture(const uint32_t AttachmentIdx, const uint32_t Slot) const
	{
		LK_TRACE_TAG("Framebuffer", "BindTexture: AttachmentIdx={} Slot={}", AttachmentIdx, Slot);
		LK_OpenGL_Verify(glBindTextureUnit(Slot, ColorAttachments[AttachmentIdx]->GetID()));
	}

	void CFramebuffer::Bind() const
	{
		LK_OpenGL_Verify(glBindFramebuffer(GL_FRAMEBUFFER, ID));
	}

	void CFramebuffer::Unbind() const
	{
		LK_OpenGL_Verify(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	}

	void CFramebuffer::Clear() const
	{
		LK_OpenGL_Verify(glClearColor(ClearColor.r, ClearColor.g, ClearColor.b, ClearColor.a));
		LK_OpenGL_Verify(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

		LK_OpenGL_Verify(glBindFramebuffer(GL_FRAMEBUFFER, ID));
		LK_OpenGL_Verify(glClearColor(ClearColor.r, ClearColor.g, ClearColor.b, ClearColor.a));
		LK_OpenGL_Verify(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
	}

	std::shared_ptr<CTexture> CFramebuffer::GetImage(const uint32_t AttachmentIdx)
	{
		LK_ASSERT(AttachmentIdx < ColorAttachments.size());
		return ColorAttachments[AttachmentIdx];
	}

	void CFramebuffer::Resize(const uint32_t InWidth, const uint32_t InHeight)
	{
		LK_PROFILE_FUNC();
		LK_TRACE_TAG("Framebuffer", "Resize: ({}, {})", InWidth, InHeight);
		if ((InWidth <= 0) || (InHeight <= 0)) {
			return;
		}
		Spec.Width = InWidth;
		Spec.Height = InHeight;
		Invalidate();
		LK_OpenGL_Verify(glViewport(0, 0, Spec.Width, Spec.Height));
	}

	int CFramebuffer::ReadPixel(const uint32_t AttachmentIndex, const int PosX, const int PosY)
	{
		LK_OpenGL_Verify(glReadBuffer(GL_COLOR_ATTACHMENT0 + AttachmentIndex));
		int PixelData;
		LK_OpenGL_Verify(glReadPixels(PosX, PosY, 1, 1, GL_RED_INTEGER, GL_INT, &PixelData));
		return PixelData;
	}

	void CFramebuffer::ClearAttachment(const uint32_t AttachmentIdx, const int Value)
	{
		LK_ASSERT(AttachmentIdx < ColorAttachments.size());
		LK_DEBUG_TAG("Framebuffer", "ClearAttachment: AttachmentIdx={} Value={}", AttachmentIdx, Value);
		FFramebufferTextureSpecification& TextureSpec = ColorAttachmentSpecs[AttachmentIdx];

		LK_OpenGL_Verify(glClearTexImage(
			ColorAttachments[AttachmentIdx]->GetID(),
			0,
			OpenGL::GetImageFormat(TextureSpec.ImageFormat),
			GL_INT,
			&Value));
	}

	uint32_t CFramebuffer::GetColorAttachmentID(const uint32_t Idx) const
	{
		LK_ASSERT(Idx < ColorAttachments.size(), "Index {} too large", Idx);
		LK_ASSERT(ColorAttachments[Idx] != nullptr);
		return ColorAttachments[Idx]->GetID();
	}

	static bool IsDepthFormat(const EImageFormat ImageFormat)
	{
		switch (ImageFormat) {
			case EImageFormat::DEPTH24STENCIL8: return true;
		}
		return false;
	}

	static GLenum GetTextureTarget(const bool Multisampled)
	{
		return (Multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D);
	}

	static void AttachDepthTexture(const uint32_t ID, const int Samples, const GLenum Format, const GLenum AttachmentType, const uint32_t Width, const uint32_t Height)
	{
		const bool Multisampled = (Samples > 1);
		if (Multisampled) {
			LK_OpenGL_Verify(glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, Samples, Format, Width, Height, GL_FALSE));
		} else {
			LK_OpenGL_Verify(glTexStorage2D(GL_TEXTURE_2D, 1, Format, Width, Height));
			LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
			LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
			LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE));
			LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
			LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		}

		LK_OpenGL_Verify(glFramebufferTexture2D(GL_FRAMEBUFFER, AttachmentType, GetTextureTarget(Multisampled), ID, 0));
	}

}
