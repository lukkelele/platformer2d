#pragma once

#include <glad/glad.h>

#include "core/assert.h"
#include "renderer/vertexbufferlayout.h"

namespace platformer2d::OpenGL_Internal 
{
	FORCEINLINE static void CheckForErrors()
	{
		while (glGetError() != GL_NO_ERROR)
		{
		}
	}

	FORCEINLINE static bool VerifyFunctionResult(const char* InFunction, const char* InFile, int InLine)
	{
		while (GLenum Error = glGetError())
		{
			std::printf("Error %d\n Function: %s\n File: %s\n Line: %d\n",
							  static_cast<int>(Error), InFunction, InFile, InLine);
			return false;
		}
		return true;
	}
}

#define LK_OpenGL_Verify(OpenGLFunction) \
	OpenGL_Internal::CheckForErrors();  \
	OpenGLFunction;                      \
	LK_VERIFY(OpenGL_Internal::VerifyFunctionResult(#OpenGLFunction, __FILE__, __LINE__))

namespace platformer2d::OpenGL {

	namespace VertexBuffer {
		static inline void Initialize(GLuint* VBO, void* Data, const std::size_t DataSize)
		{
			LK_ASSERT(VBO && Data && (DataSize > 0));
			glGenBuffers(1, VBO);
			glBindBuffer(GL_ARRAY_BUFFER, *VBO);
			glBufferData(GL_ARRAY_BUFFER, DataSize, Data, GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
	}

	FORCEINLINE static constexpr GLenum ShaderDataTypeToOpenGLBaseType(const EShaderDataType Type)
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

}
