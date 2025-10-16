#include "opengl_debug.h"

#include <glad/glad.h>

#include "core/assert.h"
#include "core/core.h"
#include "core/log.h"

#ifndef GL_VERSION_4_3
#error "Current OpenGL version not supported"
#endif

#ifndef GL_KHR_debug
#error "Missing extension: GL_KHR_debug"
#endif

namespace platformer2d::OpenGL::Internal {

	static const char* SourceToString(const GLenum Source)
	{
		switch (Source)
		{
			case GL_DEBUG_SOURCE_API:             return "API";
			case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   return "WindowSys";
			case GL_DEBUG_SOURCE_SHADER_COMPILER: return "Shader";
			case GL_DEBUG_SOURCE_THIRD_PARTY:     return "3rdParty";
			case GL_DEBUG_SOURCE_APPLICATION:     return "App";
			case GL_DEBUG_SOURCE_OTHER:           return "Other";
			default:                              return "?";
		}
	}

	static const char* TypeToString(const GLenum Type)
	{
		switch (Type)
		{
			case GL_DEBUG_TYPE_ERROR:               return "Error";
			case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: return "Deprecated";
			case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  return "Undef";
			case GL_DEBUG_TYPE_PORTABILITY:         return "Portability";
			case GL_DEBUG_TYPE_PERFORMANCE:         return "Perf";
			case GL_DEBUG_TYPE_MARKER:              return "Marker";
			case GL_DEBUG_TYPE_PUSH_GROUP:          return "PushGroup";
			case GL_DEBUG_TYPE_POP_GROUP:           return "PopGroup";
			case GL_DEBUG_TYPE_OTHER:               return "Other";
			default:                                return "?";
		}
	}

	static const char* SeverityToString(const GLenum Severity)
	{
		switch (Severity)
		{
			case GL_DEBUG_SEVERITY_HIGH:         return "HIGH";
			case GL_DEBUG_SEVERITY_MEDIUM:       return "MED";
			case GL_DEBUG_SEVERITY_LOW:          return "LOW";
			case GL_DEBUG_SEVERITY_NOTIFICATION: return "NOTE";
			default:                             return "?";
		}
	}

	static void APIENTRY DebugCallback(const GLenum Source,
									   const GLenum Type,
									   const GLuint ID,
									   const GLenum Severity,
									   const GLsizei Length,
									   const GLchar* Message,
									   const void* UserParam)
	{
		LK_UNUSED(Length);
		LK_UNUSED(UserParam);
		LK_ERROR_TAG("OpenGL", "\n * Type: {}\n"
					 " * Source: {}\n * Severity: {}\n * ID: {}\n"
					 "\n{}\n",
					 TypeToString(Type),
					 SourceToString(Source),
					 SeverityToString(Severity),
					 ID,
					 Message
		);

		if (Type == GL_DEBUG_TYPE_ERROR)
		{
			LK_DEBUG_BREAK();
		}
	}

	void SetupDebugContext(void* Ctx)
	{
		int Flags = 0;
		glGetIntegerv(GL_CONTEXT_FLAGS, &Flags);
		LK_ASSERT(Flags & GL_CONTEXT_FLAG_DEBUG_BIT);

		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);

		glDebugMessageCallback(DebugCallback, Ctx);
		glDebugMessageControl(
			GL_DONT_CARE,
			GL_DONT_CARE,
			GL_DONT_CARE,
			0,
			nullptr,
			GL_TRUE
		);
	}

}
