#pragma once

#include <glad/glad.h>

#include "core/assert.h"

namespace OpenGL_Internal 
{
	static inline void CheckForErrors()
	{
		while (glGetError() != GL_NO_ERROR)
		{
		}
	}

	static inline bool VerifyFunctionResult(const char* InFunction, const char* InFile, int InLine)
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
