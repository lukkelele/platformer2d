#include "texture.h"

#include <stb/stb_image.h>

#include "core/log.h"
#include "texturearray.h"

namespace platformer2d {

	namespace {
		std::size_t CreatedTextures = 0;
	}

	static void InvertRgba8(void* Data, const std::size_t Width, const std::size_t Height)
	{
		LK_ASSERT(Data);
		const std::size_t PixelCount = Width * Height;
		uint8_t* Bytes = static_cast<uint8_t*>(Data);
		for (std::size_t Idx = 0; Idx < PixelCount; Idx++) {
			const std::size_t Offset = Idx * 4;
			Bytes[Offset + 0] = static_cast<uint8_t>(255 - Bytes[Offset + 0]);
			Bytes[Offset + 1] = static_cast<uint8_t>(255 - Bytes[Offset + 1]);
			Bytes[Offset + 2] = static_cast<uint8_t>(255 - Bytes[Offset + 2]);
			/* Leave alpha as is. */
		}
	}

	CTexture::CTexture(const FTextureSpecification& Spec)
		: Path(Spec.Path)
		, Name(Spec.Name)
	{
		LK_ASSERT((Spec.Width > 0) && (Spec.Height > 0) && std::filesystem::exists(Spec.Path));
		LK_OpenGL_Verify(glCreateTextures(GL_TEXTURE_2D, 1, &ID));
		LK_OpenGL_Verify(glBindTexture(GL_TEXTURE_2D, ID));

		DataFormat = OpenGL::GetImageFormat(Spec.Format);
		InternalFormat = OpenGL::GetImageInternalFormat(Spec.Format);
		DataType = OpenGL::GetFormatDataType(Spec.Format);
		LK_TRACE_TAG("Texture", "Format: {} (GLFormat={} InternalGLFormat={})", Enum::ToString(Spec.Format), DataFormat, InternalFormat);

		stbi_set_flip_vertically_on_load(Spec.bFlipVertical);
		int ReadWidth, ReadHeight, ReadChannels;
		const bool IsHdr = stbi_is_hdr(Spec.Path.generic_string().c_str());

		void* Data = nullptr;
		if (IsHdr) {
			LK_TRACE_TAG("Texture", "[{}] HDR texture", Path.filename());
			Data = stbi_loadf(Spec.Path.generic_string().c_str(), &ReadWidth, &ReadHeight, &ReadChannels, 4);
		} else {
			Data = stbi_load(Spec.Path.generic_string().c_str(), &ReadWidth, &ReadHeight, &ReadChannels, 4);
		}

		LK_ASSERT(Data != NULL, "Failed to load texture from: {}", Spec.Path);
		if ((ReadWidth != Spec.Width) || (ReadHeight != Spec.Height)) {
			LK_TRACE("Texture mismatch ({}) between specified and actual size ({}x{} != {}x{})",
					 Path.filename().generic_string(), Spec.Width, Spec.Height,
					 ReadWidth, ReadHeight);
		}

		if (Spec.bInvert) {
			LK_VERIFY(!IsHdr, "Invert for HDR not supported");
			LK_TRACE_TAG("Texture", "Invert color: {}", Spec.Path);
			InvertRgba8(Data, ReadWidth, ReadHeight);
		}

		Width = ReadWidth;
		Height = ReadHeight;
		Channels = ReadChannels;
		const uint64_t ImageSize = OpenGL::CalculateImageSize(Spec.Format, Width, Height);
		LK_ASSERT(ImageSize <= std::numeric_limits<uint64_t>::max(), "ImageSize overflow");
		LK_TRACE_TAG("Texture", "[{}] Format={} Size=({}x{}) Storage={} Channels={}, GLFormat={} InternalGLFormat={}", Spec.Path.stem(), Enum::ToString(Spec.Format), Width, Height, Spec.bStorage, Channels, DataFormat, InternalFormat);
		ImageBuffer = FBuffer::Copy(Data, ImageSize);

		if (Data) {
			LK_OpenGL_Verify(glTexImage2D(
				GL_TEXTURE_2D,
				0,
				InternalFormat,
				ReadWidth,
				ReadHeight,
				0,
				DataFormat,
				DataType,
				ImageBuffer.Data
			));

			stbi_image_free(Data);
			LK_ASSERT(ImageBuffer.Data);
		}

		Mips = Spec.Mips;
		const bool bMipmap = (Spec.Mips > 1);
		if (bMipmap) {
			LK_TRACE_TAG("Texture", "[{}] Generating mipmap (Mips: {})", Path.filename(), Spec.Mips);
			LK_OpenGL_Verify(glGenerateTextureMipmap(ID));
		} else {
			LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0));
		}

		OpenGL::SetTextureWrap(Spec.SamplerWrap);
		OpenGL::SetTextureFilter(Spec.SamplerFilter, bMipmap);

		if (Name.empty()) {
			Name = Format("{}", Path.filename());
		}

		Slot = CreatedTextures++;
		LK_TRACE_TAG("Texture", "Index: {} ({})", Slot, Path.filename());
	}

	CTexture::CTexture(const FTextureSpecification& Spec, const FBuffer& InData)
		: Path(Spec.Path)
		, Name(Spec.Name)
		, Width(Spec.Width)
		, Height(Spec.Height)
	{
		LK_ASSERT((Spec.Width > 0) && (Spec.Height > 0) && !Spec.Path.empty());
		LK_OpenGL_Verify(glCreateTextures(GL_TEXTURE_2D, 1, &ID));
		LK_OpenGL_Verify(glBindTexture(GL_TEXTURE_2D, ID));

		DataFormat = OpenGL::GetImageFormat(Spec.Format);
		InternalFormat = OpenGL::GetImageInternalFormat(Spec.Format);
		DataType = OpenGL::GetFormatDataType(Spec.Format);
		LK_TRACE_TAG("Texture", "[{}] Format={} Size=({}x{}) Storage={} GLFormat={} InternalGLFormat={}", Spec.Path.stem(), Enum::ToString(Spec.Format), Width, Height, Spec.bStorage, Channels, DataFormat, InternalFormat);

		if (Spec.bStorage) {
			LK_OpenGL_Verify(glTexImage2D(
				GL_TEXTURE_2D,
				0,
				InternalFormat,
				Spec.Width,
				Spec.Height,
				0,
				DataFormat,
				DataType,
				nullptr
			));
		} else if (InData.Data) {
			ImageBuffer = FBuffer::Copy(InData);
			if (ImageBuffer.Data) {
				LK_OpenGL_Verify(glTexImage2D(
					GL_TEXTURE_2D,
					0,
					InternalFormat,
					Spec.Width,
					Spec.Height,
					0,
					DataFormat,
					DataType,
					ImageBuffer.Data
				));
			}
		}

		Mips = Spec.Mips;
		const bool bMipmap = (Spec.Mips > 1);
		if (bMipmap) {
			LK_TRACE_TAG("Texture", "[{}] Generating mipmap (Mips: {})", Path.filename(), Spec.Mips);
			LK_OpenGL_Verify(glGenerateTextureMipmap(ID));
		} else {
			LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0));
		}

		OpenGL::SetTextureWrap(Spec.SamplerWrap);
		OpenGL::SetTextureFilter(Spec.SamplerFilter, bMipmap);

		if (Name.empty()) {
			Name = Format("{}", Path.filename());
		}
		Slot = CreatedTextures++;
		LK_TRACE_TAG("Texture", "Index: {} ({})", Slot, Path.filename());
	}

	CTexture::CTexture(const uint32_t InWidth, const uint32_t InHeight, void* InData)
		: Width(InWidth)
		, Height(InHeight)
	{
		LK_ASSERT((InWidth > 0) && (InHeight > 0));
		LK_OpenGL_Verify(glGenTextures(1, &ID));
		LK_OpenGL_Verify(glBindTexture(GL_TEXTURE_2D, ID));

		/**
		 * @todo Pass texture wrap/filter args from constructor.
		 * Also pass other metadata to know if stbi_load/stbi_loadf was used
		 * to correctly map GL_FLOAT/GL_UNSIGNED_BYTE.
		 */
		static const GLenum ImageFormat = GL_RGBA;
		static const GLenum InternalImageFormat = GL_RGBA32F;
		Channels = 4;

		if (InData) {
			LK_OpenGL_Verify(glTexImage2D(
				GL_TEXTURE_2D,
				0,
				InternalImageFormat,
				Width,
				Height,
				0,
				ImageFormat,
				GL_FLOAT,
				(const void*)InData
			));

			stbi_image_free(InData);
		}

		LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
		LK_OpenGL_Verify(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

		Name = Format("{}", Path.filename());
		Slot = CreatedTextures++;
		LK_TRACE_TAG("Texture", "Index: {}", Slot);
	}

	CTexture::~CTexture()
	{
		/* @todo: Check if resources left behind */
		LK_TRACE_TAG("Texture", "Release: {} {}x{}", Name, Width, Height);
	}

	void CTexture::Bind(const uint32_t Slot) const
	{
		LK_OpenGL_Verify(glBindTextureUnit(Slot, ID));
	}

	void CTexture::Unbind(const uint32_t Slot) const
	{
		LK_OpenGL_Verify(glBindTextureUnit(Slot, 0));
	}

	void CTexture::Invalidate()
	{
		if (ID) {
			LK_OpenGL_Verify(glDeleteTextures(1, &ID));
			ID = 0;
		}

		const uint32_t MipCount = OpenGL::CalculateMipCount(Width, Height); /* @todo Use 'Mips' member here or? */
		LK_OpenGL_Verify(glCreateTextures(GL_TEXTURE_2D, 1, &ID));
		LK_OpenGL_Verify(glTextureStorage2D(
			ID,
			MipCount,
			InternalFormat,
			Width,
			Height
		));

		if (ImageBuffer) {
			LK_OpenGL_Verify(glTextureSubImage2D(
				ID,
				0,
				0,
				0,
				Width,
				Height,
				DataFormat,
				DataType,
				ImageBuffer.Data
			));
		}
	}

	void CTexture::SetWrap(const ETextureWrap InWrap) const
	{
		LK_DEBUG_TAG("Texture", "Set wrap: {} ({}) (Index {})", Enum::ToString(InWrap), Path.filename(), Slot);
		Bind(Slot);
		OpenGL::SetTextureWrap(ID, InWrap);
		Unbind(Slot);
	}

	void CTexture::SetFilter(const ETextureFilter InFilter) const
	{
		LK_DEBUG_TAG("Texture", "Set filter: {} ({}) (Index {})", Enum::ToString(InFilter), Path.filename(), Slot);
		Bind(Slot);
		OpenGL::SetTextureFilter(ID, InFilter, (Mips > 1));
		Unbind(Slot);
	}

	void CTexture::SetSlot(const std::size_t InSlot)
	{
		Slot = InSlot;
	}

}
