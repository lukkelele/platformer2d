#pragma once

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <type_traits>

#include <glad/glad.h>
#include <spdlog/spdlog.h>

static_assert(std::is_same_v<GLuint, uint32_t>, "Type mismatch");

//static uint32_t LoadShaders(const char* VertexShaderPath, const char* FragShaderPath)
static uint32_t LoadShaders(const std::filesystem::path& VertexShaderPath, const std::filesystem::path& FragmentShaderPath)
{
	/* Create shaders. */
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	/* Read vertex shader from file. */
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(VertexShaderPath, std::ios::in);
	if (VertexShaderStream.is_open())
	{
		std::stringstream ss;
		ss << VertexShaderStream.rdbuf();
		VertexShaderCode = ss.str();
		VertexShaderStream.close();
	}
	else
	{
		spdlog::error("Failed to open {} (pwd={})", VertexShaderPath.generic_string(), std::filesystem::current_path().generic_string());
		getchar();
		return 0;
	}

	/* Read fragment shader. */
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(FragmentShaderPath, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::stringstream ss;
		ss << FragmentShaderStream.rdbuf();
		FragmentShaderCode = ss.str();
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	/* Compile vertex shader. */
	spdlog::info("Compiling: {}", VertexShaderPath.generic_string());
	const char* VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	/* Check vertex shader. */
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0)
	{
		std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		spdlog::error("{}", &VertexShaderErrorMessage[0]);
	}

	/* Compile fragment shader. */
	spdlog::info("Compiling: {}", FragmentShaderPath.generic_string());
	const char* FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	/* Check fragment shader. */
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0)
	{
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength+1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		spdlog::error("{}", &FragmentShaderErrorMessage[0]);
	}

	/* Link the program. */
	spdlog::info("Linking shader program");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	/* Check the program. */
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0)
	{
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		spdlog::error("{}", &ProgramErrorMessage[0]);
	}
	
	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);
	
	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}
