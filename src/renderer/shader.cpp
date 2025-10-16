#include "shader.h"

#include <fstream>
#include <sstream>

#include <spdlog/spdlog.h>

#include "core/assert.h"

namespace platformer2d {

	static_assert(std::is_same_v<uint32_t, GLuint>);
	static_assert(std::is_same_v<float, GLfloat>);

	/**
	 * @brief Read contents of a shader file.
	 * @returns Number of lines read from file.
	 */
	static uint16_t ReadShaderFile(const std::filesystem::path& ShaderPath, std::string& Source)
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

	CShader::CShader(const std::filesystem::path& ShaderPath)
		: Filepath(ShaderPath)
	{
		FShaderProgramSource Source{};
		const bool bParsed = ParseShader(ShaderPath, Source);
		LK_VERIFY(bParsed == true, "Failed to parse shader: {}", ShaderPath.generic_string());

#ifdef LK_SHADER_LOG_PROGRAM_SOURCE
		LK_INFO("\nVertex:\n{}\n", Source.Vertex);
		LK_INFO("Fragment:\n{}\n", Source.Fragment);
#endif

		uint32_t Program;
		LK_OpenGL_Verify(Program = glCreateProgram());

		const uint32_t VertexShader = CompileShader(GL_VERTEX_SHADER, Source.Vertex);
		const uint32_t FragShader = CompileShader(GL_FRAGMENT_SHADER, Source.Fragment);

		LK_ASSERT((VertexShader != 0) && (FragShader != 0));
		LK_OpenGL_Verify(glAttachShader(Program, VertexShader));
		LK_OpenGL_Verify(glAttachShader(Program, FragShader));

		/* Link and validate. */
		LK_OpenGL_Verify(glLinkProgram(Program));
		LK_OpenGL_Verify(glValidateProgram(Program));

		auto VerifyShaderProgram = [](const GLuint Shader) -> bool
		{
			int InfoLogLength;
			LK_OpenGL_Verify(glGetShaderiv(Shader, GL_INFO_LOG_LENGTH, &InfoLogLength));
			if (InfoLogLength > 0)
			{
				std::vector<char> ErrorMsg(InfoLogLength + 1);
				glGetShaderInfoLog(Shader, InfoLogLength, NULL, &ErrorMsg[0]);
			}
			return (InfoLogLength == 0);
		};

		LK_VERIFY(VerifyShaderProgram(VertexShader));
		LK_VERIFY(VerifyShaderProgram(FragShader));

		/* Delete shader resources after shader programs are created and validated. */
		LK_OpenGL_Verify(glDeleteShader(VertexShader));
		LK_OpenGL_Verify(glDeleteShader(FragShader));

		RendererID = Program;
	}

	CShader::CShader(const std::filesystem::path& VertexShaderPath, const std::filesystem::path& FragShaderPath)
	{
		static_assert(std::is_same_v<uint32_t, GLuint>, "GLuint type mismatch");
		LK_VERIFY(std::filesystem::exists(VertexShaderPath), "Vertex shader does not exist");
		LK_VERIFY(std::filesystem::exists(FragShaderPath), "Fragment shader does not exist");

		/* Assign parent path to signal that this constructor was used. */
		Filepath = VertexShaderPath.parent_path();

		uint32_t Program;
		LK_OpenGL_Verify(Program = glCreateProgram());

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

	void CShader::Bind() const
	{
		LK_OpenGL_Verify(glUseProgram(RendererID));
	}

	void CShader::Unbind() const
	{
		LK_OpenGL_Verify(glUseProgram(0));
	}

	void CShader::Get(std::string_view Uniform, glm::vec2& Value)
	{
		float ValueArray[2] = { 0 };
		LK_OpenGL_Verify(glGetUniformfv(RendererID, GetUniformLocation(Uniform.data()), ValueArray));
		Value.x = ValueArray[0];
		Value.y = ValueArray[1];
	}

	void CShader::Get(std::string_view Uniform, glm::vec3& Value)
	{
		float ValueArray[3] = { 0 };
		LK_OpenGL_Verify(glGetUniformfv(RendererID, GetUniformLocation(Uniform.data()), ValueArray));
		Value.x = ValueArray[0];
		Value.y = ValueArray[1];
		Value.z = ValueArray[2];
	}

	void CShader::Get(std::string_view Uniform, glm::vec4& Value)
	{
		LK_OpenGL_Verify(glUseProgram(RendererID));
#if 0
		LK_OpenGL_Verify(glGetUniformfv(RendererID, GetUniformLocation(Uniform.data()), &Value.x));
#else
		float ValueArray[4] = { 0 };
		LK_OpenGL_Verify(glGetUniformfv(RendererID, GetUniformLocation(Uniform.data()), ValueArray));
		Value.x = ValueArray[0];
		Value.y = ValueArray[1];
		Value.z = ValueArray[2];
		Value.w = ValueArray[3];
#endif
	}

	void CShader::Set(std::string_view Uniform, const int Value)
	{
		LK_OpenGL_Verify(glUseProgram(RendererID));
		LK_OpenGL_Verify(glUniform1i(GetUniformLocation(Uniform.data()), Value));
	}

	void CShader::Set(std::string_view Uniform, const uint32_t Value)
	{
		LK_OpenGL_Verify(glUseProgram(RendererID));
		LK_OpenGL_Verify(glUniform1ui(GetUniformLocation(Uniform.data()), Value));
	}

	void CShader::Set(std::string_view Uniform, const float Value)
	{
		LK_OpenGL_Verify(glUseProgram(RendererID));
		LK_OpenGL_Verify(glUniform1f(GetUniformLocation(Uniform.data()), Value));
	}

	void CShader::Set(std::string_view Uniform, const bool Value)
	{
		LK_OpenGL_Verify(glUseProgram(RendererID));
		LK_OpenGL_Verify(glUniform1i(GetUniformLocation(Uniform.data()), static_cast<int>(Value)));
	}

	void CShader::Set(std::string_view Uniform, const glm::vec2& Value)
	{
		LK_OpenGL_Verify(glUseProgram(RendererID));
		LK_OpenGL_Verify(glUniform2f(GetUniformLocation(Uniform.data()), Value.x, Value.y));
	}

	void CShader::Set(std::string_view Uniform, const glm::vec3& Value)
	{
		LK_OpenGL_Verify(glUseProgram(RendererID));
		LK_OpenGL_Verify(glUniform3f(GetUniformLocation(Uniform.data()), Value.x, Value.y, Value.z));
	}

	void CShader::Set(std::string_view Uniform, const glm::vec4& Value)
	{
		LK_OpenGL_Verify(glUseProgram(RendererID));
		LK_OpenGL_Verify(glUniform4f(GetUniformLocation(Uniform.data()), Value.x, Value.y, Value.z, Value.w));
	}

	void CShader::Set(std::string_view Uniform, const glm::mat4& Value)
	{
		LK_OpenGL_Verify(glUseProgram(RendererID));
		LK_OpenGL_Verify(glUniformMatrix4fv(GetUniformLocation(Uniform.data()), 1, GL_FALSE, &Value[0][0]));
	}

	int CShader::GetUniformLocation(const std::string& Uniform)
	{
		if (UniformLocationCache.find(Uniform) != UniformLocationCache.end())
		{
			return UniformLocationCache[Uniform];
		}

		int UniformLocation;
		LK_OpenGL_Verify(UniformLocation = glGetUniformLocation(RendererID, Uniform.c_str()));
		if (UniformLocation != -1)
		{
			UniformLocationCache[Uniform] = UniformLocation;
		}
		else
		{
			LK_WARN_TAG("Shader", "Uniform '{}' is not in use ({})", Uniform, Filepath.filename());
		}

		return UniformLocation;
	}

	uint32_t CShader::CompileShader(const uint32_t ShaderType, const std::string& ShaderSource)
	{
		uint32_t ShaderID;
		LK_OpenGL_Verify(ShaderID = glCreateShader(ShaderType));

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
			spdlog::error("Failed to compile {} shader\nError: \"{}\"",
							  ((ShaderType == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT"), ErrorMessage);
			LK_OpenGL_Verify(glDeleteShader(ShaderID));
			return 0;
		}

		return ShaderID;
	}

	bool CShader::ParseShader(const std::filesystem::path& Filepath, FShaderProgramSource& Source)
	{
		std::stringstream StringStreams[2];
		EShaderType ShaderType = EShaderType::None;

		std::ifstream InputStream(Filepath);
		std::string Line;
		while (std::getline(InputStream, Line))
		{
			if (Line.find("#lk_shader") != std::string::npos)
			{
				if (Line.find("vertex") != std::string::npos)
				{
					ShaderType = EShaderType::Vertex;
				}
				else if (Line.find("fragment") != std::string::npos)
				{
					ShaderType = EShaderType::Fragment;
				}
			}
			else
			{
				if (ShaderType != EShaderType::None)
				{
					StringStreams[static_cast<int>(ShaderType)] << Line << '\n';
				}
			}
		}

		Source.Vertex = StringStreams[0].str();
		Source.Fragment = StringStreams[1].str();

		return Source.IsValid();
	}

}