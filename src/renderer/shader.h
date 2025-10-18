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

		void Bind() const;
		void Unbind() const;

		FORCEINLINE LRendererID GetRendererID() const { return RendererID; }
		const std::filesystem::path& GetFilepath() const { return Filepath; }

		int GetUniformLocation(const std::string& Uniform);

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

		template<std::size_t N>
		void Set(std::string_view Uniform, const std::array<int, N>& Value)
		{
			static_assert(N > 0);
			LK_OpenGL_Verify(glUseProgram(RendererID));
			LK_OpenGL_Verify(glUniform1iv(GetUniformLocation(Uniform.data()), N, Value.data()));
		}

		template<std::size_t N>
		void Set(std::string_view Uniform, const std::array<uint32_t, N>& Value)
		{
			static_assert(N > 0);
			LK_OpenGL_Verify(glUseProgram(RendererID));
			LK_OpenGL_Verify(glUniform1ui(GetUniformLocation(Uniform.data()), N, Value.data()));
		}

		template<typename T>
		void Set(std::string_view Uniform, const T* Array, const std::size_t ArrSize)
		{
			static_assert(std::disjunction_v<std::is_same<T, int>,
											 std::is_same<T, uint32_t>>);
			LK_OpenGL_Verify(glUseProgram(RendererID));
			if constexpr (std::is_same_v<T, int>)
			{
				LK_OpenGL_Verify(glUniform1iv(GetUniformLocation(Uniform.data()), ArrSize, Array));
			}
			else if constexpr (std::is_same_v<T, uint32_t>)
			{
				LK_OpenGL_Verify(glUniform1ui(GetUniformLocation(Uniform.data()), ArrSize, Array));
			}
		}

	private:
		uint32_t CompileShader(uint32_t ShaderType, const std::string& ShaderSource);
		bool ParseShader(const std::filesystem::path& Filepath, FShaderProgramSource& Source);

	private:
		LRendererID RendererID;
		std::unordered_map<std::string, int> UniformLocationCache{};
		std::filesystem::path Filepath;
	};
}