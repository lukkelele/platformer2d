#pragma once

#include <glm/glm.hpp>

#include "core/core.h"
#include "core/log.h"
#include "shader.h"
#include "texture.h"
#include "imguilayer.h"

namespace platformer2d {

	struct FQuadVertex
	{
		glm::vec3 Position{};
		glm::vec4 Color{};
		glm::vec2 TexCoord{};
		float TexIndex{};
		float TileFactor{};
	};

	struct FLineVertex
	{
		glm::vec3 Position{};
		glm::vec4 Color{};
	};

	class CRenderer
	{
	public:
		CRenderer() = delete;
		~CRenderer() = delete;
		CRenderer(const CRenderer&) = delete;
		CRenderer(CRenderer&&) = delete;

		static void Initialize();
		static void Destroy();
		static void BeginFrame();
		static void EndFrame();

		static void StartBatch();
		static void NextBatch();
		static void Flush();

		static void DrawQuad(const glm::vec2& Pos, const glm::vec2& Size, 
							 const glm::vec4& Color = {1.0f, 1.0f, 1.0f, 1.0f}, float RotationDeg = 0.0f);
		static void DrawLine(const glm::vec2& P1, const glm::vec2& P2, uint16_t LineWidth = 8,
							 const glm::vec4& Color = { 1.0f, 1.0f, 1.0f, 1.0f });

		static void SetClearColor(const glm::vec4& InClearColor) { ClearColor = InClearColor; }

	private:
		CRenderer& operator=(const CRenderer&) = delete;
		CRenderer& operator=(CRenderer&&) = delete;

	private:
		inline static glm::vec4 ClearColor{ 0.20f, 0.20f, 0.20f, 1.0f };
		inline static std::unique_ptr<CImGuiLayer> ImGuiLayer = nullptr;

		inline static GLuint QuadVAO = 0;
		inline static GLuint QuadVBO = 0;
		inline static GLuint QuadEBO = 0;
		inline static uint32_t QuadIndexCount = 0;
		inline static glm::vec4 QuadVertexPositions[4] = {};
		inline static FQuadVertex* QuadVertexBufferBase = nullptr;
		inline static FQuadVertex* QuadVertexBufferPtr = nullptr;
		inline static std::unique_ptr<CShader> QuadShader = nullptr;
	};

}