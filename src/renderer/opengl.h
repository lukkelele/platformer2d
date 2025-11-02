#pragma once

#include <array>

#include <glad/glad.h>

#include "core/assert.h"
#include "backendinfo.h"
#include "texture_enums.h"
#include "vertexbufferlayout.h"

#ifdef LK_BUILD_DEBUG
#include "opengl_debug.h"
#endif

#define LK_VERTEXARRAY_RETURN_STD_ARRAY 1

#define LK_OpenGL_Verify(OpenGLFunction) \
	::platformer2d::OpenGL::Internal::CheckForErrors(); \
	OpenGLFunction; \
	LK_VERIFY(::platformer2d::OpenGL::Internal::VerifyFunctionResult(#OpenGLFunction, __FILE__, __LINE__))

namespace platformer2d {

	/**
	 * @enum EPrimitiveTopology
	 */
	enum class EPrimitiveTopology
	{
		None = 0,
		Points,
		Lines,
		Triangles,
		LineStrip,
		TriangleStrip,
		TriangleFan
	};

	namespace OpenGL::Internal
	{
		inline void CheckForErrors()
		{
			while (glGetError() != GL_NO_ERROR);
		}

		inline bool VerifyFunctionResult(const char* InFunction, const char* InFile, int InLine)
		{
			while (GLenum Error = glGetError())
			{
				std::printf("Error: %d\n * Function: %s\n * File: %s\n * Line: %d\n",
								  static_cast<int>(Error), InFunction, InFile, InLine);
				return false;
			}
			return true;
		}
	}
}

namespace platformer2d::OpenGL {

	void LoadInfo(FBackendInfo& Info);
	void SetTextureFilter(ETextureFilter TextureFilter, bool IsMipmap = false);
	void SetTextureFilter(LRendererID ID, ETextureFilter Filter, bool IsMipmap = false);
	void SetTextureWrap(ETextureWrap TextureWrap);
	void SetTextureWrap(LRendererID ID, ETextureWrap TextureWrap);
	GLenum GetImageFormat(EImageFormat Format);
	GLenum GetFormatDataType(EImageFormat Format);
	GLenum GetImageInternalFormat(EImageFormat Format);
	GLenum GetSamplerWrap(ETextureWrap TextureWrap);
	GLenum GetSamplerFilter(ETextureFilter TextureFilter, bool IsMipmap);
	GLenum ImageFormatToDataFormat(EImageFormat Format);

	uint32_t CalculateMipCount(uint32_t Width, uint32_t Height);

	/**
	 * @brief Get bytes per pixel.
	 */
	uint32_t GetFormatBPP(EImageFormat ImageFormat);
	uint32_t CalculateImageSize(EImageFormat ImageFormat, uint32_t Width, uint32_t Height);

	static constexpr GLenum ShaderDataTypeToOpenGLBaseType(const EShaderDataType Type)
	{
		switch (Type)
		{
			case EShaderDataType::Float:  return GL_FLOAT;
			case EShaderDataType::Float2: return GL_FLOAT;
			case EShaderDataType::Float3: return GL_FLOAT;
			case EShaderDataType::Float4: return GL_FLOAT;
			case EShaderDataType::Mat3:	  return GL_FLOAT;
			case EShaderDataType::Mat4:	  return GL_FLOAT;
			case EShaderDataType::Int:
			case EShaderDataType::Int2:
			case EShaderDataType::Int3:
			case EShaderDataType::Int4:	  return GL_INT;
			case EShaderDataType::Bool:	  return GL_BOOL;
		}
		return GL_INVALID_ENUM;
	}

	static void ApplyVertexBufferLayout(const FVertexBufferLayout& Layout)
	{
		int VertexBufferIndex = 0;
		for (const FVertexBufferElement& Element : Layout)
		{
			LK_TRACE("ComponentCount={} Stride={}", Element.GetComponentCount(), Layout.GetStride());
			switch (Element.Type)
			{
				case EShaderDataType::Float:
				case EShaderDataType::Float2:
				case EShaderDataType::Float3:
				case EShaderDataType::Float4:
				{
					glEnableVertexAttribArray(VertexBufferIndex);
					glVertexAttribPointer(
						VertexBufferIndex,
						Element.GetComponentCount(),
						OpenGL::ShaderDataTypeToOpenGLBaseType(Element.Type),
						(Element.Normalized ? GL_TRUE : GL_FALSE),
						Layout.GetStride(),
						(const void*)Element.Offset
					);
					VertexBufferIndex++;
					break;
				}
				case EShaderDataType::Int:
				case EShaderDataType::Int2:
				case EShaderDataType::Int3:
				case EShaderDataType::Int4:
				case EShaderDataType::Bool:
				{
					glEnableVertexAttribArray(VertexBufferIndex);
					glVertexAttribIPointer(
						VertexBufferIndex,
						Element.GetComponentCount(),
						OpenGL::ShaderDataTypeToOpenGLBaseType(Element.Type),
						Layout.GetStride(),
						(const void*)Element.Offset
					);
					VertexBufferIndex++;
					break;
				}
				case EShaderDataType::Mat3:
				case EShaderDataType::Mat4:
				{
					const uint8_t Count = Element.GetComponentCount();
					for (uint8_t Idx = 0; Idx < Count; Idx++)
					{
						glEnableVertexAttribArray(VertexBufferIndex);
						glVertexAttribPointer(
							VertexBufferIndex,
							Count,
							OpenGL::ShaderDataTypeToOpenGLBaseType(Element.Type),
							(Element.Normalized ? GL_TRUE : GL_FALSE),
							Layout.GetStride(),
							(const void*)(Element.Offset + sizeof(float) * Count * Idx)
						);
						glVertexAttribDivisor(VertexBufferIndex, 1);
						VertexBufferIndex++;
					}
					break;
				}

				default:
					LK_ERROR("Unhandled shader type: {}", Enum::ToString(Element.Type));
			}
		}
	}

	namespace VertexArray 
	{
		template<std::size_t N = 1>
		static auto Create()
		{
			/* Return type: GLuint */
			if constexpr (N == 1)
			{
				GLuint VAO;
				LK_OpenGL_Verify(glGenVertexArrays(1, &VAO));
				LK_OpenGL_Verify(glBindVertexArray(VAO));
				return VAO;
			}
			/* Return type: std::array<GLuint, N> */
			else
			{
			#if LK_VERTEXARRAY_RETURN_STD_ARRAY
				std::array<GLuint, N> VAO;
				LK_OpenGL_Verify(glGenVertexArrays(static_cast<GLsizei>(N), VAO.data()));
			#else
				GLuint VAO[N];
				LK_OpenGL_Verify(glBindVertexArray(VAO[0]));
			#endif
				return VAO;
			}
		}
	}

	namespace VertexBuffer
	{
		static GLuint Create(const std::size_t DataSize, const FVertexBufferLayout& Layout, const int BufferType = GL_DYNAMIC_DRAW)
		{
			GLuint VBO;
			LK_OpenGL_Verify(glGenBuffers(1, &VBO));
			LK_OpenGL_Verify(glBindBuffer(GL_ARRAY_BUFFER, VBO));
			LK_OpenGL_Verify(glBufferData(GL_ARRAY_BUFFER, DataSize, nullptr, BufferType));
			LK_DEBUG_TAG("VertexBuffer", "VBO={} Size={}", VBO, DataSize);
			ApplyVertexBufferLayout(Layout);
			return VBO;
		}

		template<typename T, std::size_t N>
		static GLuint Create(const T (&Data)[N], const FVertexBufferLayout& Layout, const int BufferType = GL_DYNAMIC_DRAW)
		{
			GLuint VBO;
			LK_OpenGL_Verify(glGenBuffers(1, &VBO));
			LK_OpenGL_Verify(glBindBuffer(GL_ARRAY_BUFFER, VBO));
			LK_OpenGL_Verify(glBufferData(GL_ARRAY_BUFFER, sizeof(Data), Data, BufferType));
			LK_DEBUG_TAG("VertexBuffer", "VBO={} Size={}", VBO, sizeof(Data));
			ApplyVertexBufferLayout(Layout);
			return VBO;
		}
	}

	namespace ElementBuffer
	{
		template<typename T = uint32_t, std::size_t N>
		static GLuint Create(const T (&Data)[N])
		{
			static_assert(N > 0);
			GLuint EBO;
			LK_OpenGL_Verify(glGenBuffers(1, &EBO));
			LK_OpenGL_Verify(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO));
			LK_OpenGL_Verify(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Data), Data, GL_STATIC_DRAW));
			return EBO;
		}

		template<typename T = uint32_t>
		static GLuint Create(const T* Data, const std::size_t Size)
		{
			LK_ASSERT(Size > 0);
			GLuint EBO;
			LK_OpenGL_Verify(glGenBuffers(1, &EBO));
			LK_OpenGL_Verify(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO));
			LK_OpenGL_Verify(glBufferData(GL_ELEMENT_ARRAY_BUFFER, Size, Data, GL_STATIC_DRAW));
			return EBO;
		}


		template<typename T = uint32_t, std::size_t N>
		static GLuint Create(const std::array<T, N>& Data)
		{
			return Create(Data.data());
		}
	}

}
