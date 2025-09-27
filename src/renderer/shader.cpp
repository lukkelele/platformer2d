#include "shader.h"

#include <fstream>
#include <sstream>

#include <spdlog/spdlog.h>

namespace platformer2d {

	/**
	 * @brief Read contents of a shader file.
	 * @returns Number of lines read from file.
	 */
	static inline uint16_t ReadShaderFile(const std::filesystem::path& ShaderPath, std::string& Source)
	{
		std::ifstream File(ShaderPath, std::ios::in);
		if (!File.is_open())
		{
			spdlog::error("Failed to open: {}", ShaderPath.generic_string());
			return 0;
		}

		Source.clear();
		uint16_t Count = 0;
		std::string Line;
		std::stringstream SStream;
		while (std::getline(File, Line))
		{
			SStream << Line << '\n';
			Count++;
		}

		Source = SStream.str();
		File.close();

		return Count;
	}

	CShader::CShader(const std::filesystem::path& VertexShaderPath, const std::filesystem::path& FragShaderPath)
	{
		static_assert(std::is_same_v<uint32_t, GLuint>, "GLuint type mismatch");
		uint32_t Program;
		LK_OpenGL_Verify(Program = glCreateProgram());
		spdlog::debug("Program={}", Program);

		std::string VertexSource;
		const uint16_t VertexShaderLines = ReadShaderFile(VertexShaderPath, VertexSource);
		LK_VERIFY(VertexShaderLines > 0, "Failed reading vertex shader");
		const uint32_t VertexShader = CompileShader(GL_VERTEX_SHADER, VertexSource);

		std::string FragSource;
		const uint16_t FragShaderLines = ReadShaderFile(FragShaderPath, FragSource);
		LK_VERIFY(FragShaderLines > 0, "Failed reading fragment shader");
		const uint32_t FragShader = CompileShader(GL_FRAGMENT_SHADER, FragSource);

		/* Attach shaders. */
		LK_ASSERT((VertexShader != 0) && (FragShader != 0));
		LK_OpenGL_Verify(glAttachShader(Program, VertexShader));
		LK_OpenGL_Verify(glAttachShader(Program, FragShader));

		/* Link and validate. */
		LK_OpenGL_Verify(glLinkProgram(Program));
		LK_OpenGL_Verify(glValidateProgram(Program));

		/* Delete shader resources after shader programs are created and validated. */
		LK_OpenGL_Verify(glDeleteShader(VertexShader));
		LK_OpenGL_Verify(glDeleteShader(FragShader));

		RendererID = Program;
	}

	uint32_t CShader::CompileShader(const uint32_t ShaderType, const std::string& ShaderSource)
	{
		static_assert(std::is_same_v<uint32_t, GLuint>, "GLuint type mismatch");
		uint32_t ShaderID;
		LK_OpenGL_Verify(ShaderID = glCreateShader(ShaderType));
		spdlog::debug("Type={} ShaderID={}", ShaderType, ShaderID);

		const char* Source = ShaderSource.c_str();
		LK_OpenGL_Verify(glShaderSource(ShaderID, 1, &Source, nullptr));
		LK_OpenGL_Verify(glCompileShader(ShaderID));

		int Result;
		LK_OpenGL_Verify(glGetShaderiv(ShaderID, GL_COMPILE_STATUS, &Result));
		if (Result == GL_FALSE)
		{
			int Length;
			LK_OpenGL_Verify(glGetShaderiv(ShaderID, GL_INFO_LOG_LENGTH, &Length));

		#if defined(LK_COMPILER_MSVC)
			char* ErrorMessage = (char*)_malloca(Length * sizeof(char));
		#elif defined(LK_COMPILER_GCC) || defined(LK_COMPILER_CLANG)
			char* ErrorMessage = (char*)alloca(Length * sizeof(char));
		#endif
			LK_OpenGL_Verify(glGetShaderInfoLog(ShaderID, Length, &Length, ErrorMessage));
			spdlog::error("Failed to compile {} shader at {}, \"{}\"", 
							  ((ShaderType == GL_VERTEX_SHADER) ? "vertex" : "fragment"), FilePath.string(), ErrorMessage);
			LK_OpenGL_Verify(glDeleteShader(ShaderID));
			return 0;
		}

		return ShaderID;
	}

}