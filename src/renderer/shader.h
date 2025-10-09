#pragma once

#include <filesystem>
#include <unordered_map>

#include <glm/glm.hpp>

#include "core/assert.h"
#include "core/core.h"
#include "core/log.h"

#include "opengl.h"

namespace platformer2d {

	enum class EShaderType
	{
		None = -1,
		Vertex,
		Fragment
	};

	struct FShaderProgramSource
	{
		std::string Vertex{};
		std::string Fragment{};

		bool IsValid() const
		{
			return (!Vertex.empty() && !Fragment.empty());
		}
	};

	class CShader
	{
	public:
		CShader(const std::filesystem::path& ShaderPath);
		CShader(const std::filesystem::path& VertexShaderPath, const std::filesystem::path& FragShaderPath);
		CShader() = delete;
		~CShader() = default;

		FORCEINLINE void Bind() const { LK_OpenGL_Verify(glUseProgram(RendererID)); }
		FORCEINLINE void Unbind() const { LK_OpenGL_Verify(glUseProgram(0)); }
		FORCEINLINE LRendererID GetProgramID() const { return RendererID; }

		void Get(std::string_view Uniform, glm::vec2& Value);
		void Get(std::string_view Uniform, glm::vec3& Value);
		void Get(std::string_view Uniform, glm::vec4& Value);
		void Set(std::string_view Uniform, int Value);
		void Set(std::string_view Uniform, uint32_t Value);
		void Set(std::string_view Uniform, float Value);
		void Set(std::string_view Uniform, bool Value);
		void Set(std::string_view Uniform, const glm::vec2& Value);
		void Set(std::string_view Uniform, const glm::vec3& Value);
		void Set(std::string_view Uniform, const glm::vec4& Value);
		void Set(std::string_view Uniform, const glm::mat4& Value);

		FORCEINLINE int GetUniformLocation(const std::string& Uniform)
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
				LK_WARN("Uniform '{}' is not in use", Uniform);
			}

			return UniformLocation;
		}

	private:
		uint32_t CompileShader(uint32_t ShaderType, const std::string& ShaderSource);
		bool ParseShader(const std::filesystem::path& Filepath, FShaderProgramSource& Source);

	private:
		LRendererID RendererID;
		std::unordered_map<std::string, int> UniformLocationCache{};
	};
}