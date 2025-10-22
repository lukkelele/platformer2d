#pragma once

#include <glm/glm.hpp>

#include "shader.h"
#include "texture.h"
#include "uniformbuffer.h"

namespace platformer2d {

	class CDebugRenderer
	{
	public:
		CDebugRenderer() = delete;
		~CDebugRenderer() = delete;
		CDebugRenderer(const CDebugRenderer&) = delete;
		CDebugRenderer(CDebugRenderer&&) = delete;

		static void Initialize();
		static void Destroy();

		static void DrawQuad(const glm::vec2& Pos, const glm::vec2& Size, const glm::vec4& Color, float RotationDeg = 0.0f);

		static void DrawLine(const glm::vec2& P0, const glm::vec2& P1, const glm::vec4& Color, uint16_t LineWidth = 8);
		static void DrawLine(const glm::vec3& P0, const glm::vec3& P1, const glm::vec4& Color, uint16_t LineWidth = 8);

	private:
		CDebugRenderer& operator=(const CDebugRenderer&) = delete;
		CDebugRenderer& operator=(CDebugRenderer&&) = delete;

	private:
		static inline GLuint QuadVAO = 0;
		static inline GLuint QuadVBO = 0;
		static inline GLuint QuadEBO = 0;
		static inline std::shared_ptr<CShader> QuadShader = nullptr;

		static inline GLuint LineVAO = 0;
		static inline GLuint LineVBO = 0;
		static inline std::shared_ptr<CShader> LineShader = nullptr;
		struct FLineConfig {
			uint16_t Width = 2;
		} static inline LineConfig;
	};

}