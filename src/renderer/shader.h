#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

#include <spdlog/spdlog.h>

#include "core/core.h"
#include "core/assert.h"

#include "opengl.h"

namespace platformer2d {

	class CShader
	{
	public:
		CShader(const std::filesystem::path& VertexShaderPath, const std::filesystem::path& FragShaderPath);
		CShader() = delete;
		~CShader() = default;

		inline void Bind() const
		{
			LK_OpenGL_Verify(glUseProgram(RendererID));
		}

		inline virtual void Unbind() const
		{
			LK_OpenGL_Verify(glUseProgram(0));
		}

	private:
		uint32_t CompileShader(uint32_t ShaderType, const std::string& ShaderSource);

		inline int GetUniformLocation(const std::string& Uniform)
		{
			if (UniformLocationCache.find(Uniform) != UniformLocationCache.end())
			{
				return UniformLocationCache[Uniform];
			}

			int UniformLocation;
			LK_OpenGL_Verify(UniformLocation = glGetUniformLocation(RendererID, Uniform.c_str()));
			if (UniformLocation == -1)
			{
				spdlog::warn("Uniform '{}' is not in use ({})", Uniform, FilePath.filename().string());
			}

			/* TODO: Should caching be placed in 'else' statement here? */
			UniformLocationCache[Uniform] = UniformLocation;

			return UniformLocation;
		}

	private:
		LRendererID RendererID;
		std::unordered_map<std::string, int> UniformLocationCache{};
		std::filesystem::path FilePath{};
	};
}