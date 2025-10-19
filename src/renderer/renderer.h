#pragma once

#include <glm/glm.hpp>

#include "core/core.h"
#include "backendinfo.h"
#include "camera.h"
#include "imguilayer.h"
#include "shader.h"
#include "texture.h"
#include "uniformbuffer.h"

namespace platformer2d {

	struct FQuadVertex
	{
		glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
		glm::vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
		glm::vec2 TexCoord = { 0.0f, 0.0f };
		int TexIndex = 0;
		float TileFactor = 1.0f;
	};

	struct FLineVertex
	{
		glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
		glm::vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
	};

	struct FCircleVertex
	{
		glm::vec3 WorldPosition = { 0.0f, 0.0f, 0.0f };
		float Thickness = 1.0f;
		glm::vec2 LocalPosition = { 0.0f, 0.0f };
		glm::vec4 Color = { 1.0f, 1.0f, 1.0f, 1.0f };
	};

	struct FDrawStatistics
	{
		uint64_t QuadCount = 0;
		uint64_t LineCount = 0;
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

		static void BeginScene(const CCamera& Camera);
		static void BeginScene(const CCamera& Camera, const glm::mat4& Transform);
		static void EndScene();

		static void StartBatch();
		static void NextBatch();
		static void Flush();

		static void DrawQuad(const glm::vec2& Pos, const glm::vec2& Size, const glm::vec4& Color, float RotationDeg = 0.0f);
		static void DrawQuad(const glm::vec2& Pos, const glm::vec2& Size, CTexture& Texture, const glm::vec4& Color = {1.0f, 1.0f, 1.0f, 0.0f}, float RotationDeg = 0.0f);

		static void DrawLine(const glm::vec2& P0, const glm::vec2& P1, const glm::vec4& Color, uint16_t LineWidth = 8);
		static void DrawLine(const glm::vec3& P0, const glm::vec3& P1, const glm::vec4& Color, uint16_t LineWidth = 8);

		static void DrawCircle(const glm::vec2& P0, const glm::vec3& Rotation, float Radius, const glm::vec4& Color);
		static void DrawCircle(const glm::vec3& P0, const glm::vec3& Rotation, float Radius, const glm::vec4& Color);
		static void DrawCircle(const glm::mat4& Transform, const glm::vec4& Color);
		static void DrawCircleFilled(const glm::vec2& P0, float Radius, const glm::vec4& Color, float Thickness = 1.0f);
		static void DrawCircleFilled(const glm::vec3& P0, float Radius, const glm::vec4& Color, float Thickness = 1.0f);

		static void SetClearColor(const glm::vec4& InClearColor) { ClearColor = InClearColor; }
		static void SetLineWidth(uint16_t LineWidth);

		static const FBackendInfo& GetBackendInfo() { return BackendInfo; }

		static const FDrawStatistics& GetDrawStatistics();
		static void ResetDrawStatistics();

	private:
		CRenderer& operator=(const CRenderer&) = delete;
		CRenderer& operator=(CRenderer&&) = delete;

	private:
		static inline FBackendInfo BackendInfo;
		static inline glm::vec4 ClearColor{ 0.20f, 0.20f, 0.20f, 1.0f };
		static inline std::unique_ptr<CImGuiLayer> ImGuiLayer = nullptr;

		static inline GLuint QuadVAO = 0;
		static inline GLuint QuadVBO = 0;
		static inline GLuint QuadEBO = 0;
		static inline uint32_t QuadIndexCount = 0;
		constexpr static inline glm::vec4 QuadVertexPositions[4] = {
			{ -0.50f, -0.50f, 0.0f, 1.0f },
			{ -0.50f,  0.50f, 0.0f, 1.0f },
			{  0.50f,  0.50f, 0.0f, 1.0f },
			{  0.50f, -0.50f, 0.0f, 1.0f }
		};
		static inline FQuadVertex* QuadVertexBufferBase = nullptr;
		static inline FQuadVertex* QuadVertexBufferPtr = nullptr;
		static inline std::shared_ptr<CShader> QuadShader = nullptr;

		static inline GLuint LineVAO = 0;
		static inline GLuint LineVBO = 0;
		static inline GLuint LineEBO = 0;
		static inline uint32_t LineIndexCount = 0;
		static inline FLineVertex* LineVertexBufferBase = nullptr;
		static inline FLineVertex* LineVertexBufferPtr = nullptr;
		static inline std::shared_ptr<CShader> LineShader = nullptr;
		struct FLineConfig {
			uint16_t Width = 2;
		} static inline LineConfig;

		static inline GLuint CircleVAO = 0;
		static inline GLuint CircleVBO = 0;
		static inline GLuint CircleEBO = 0;
		static inline uint32_t CircleIndexCount = 0;
		static inline FCircleVertex* CircleVertexBufferBase = nullptr;
		static inline FCircleVertex* CircleVertexBufferPtr = nullptr;
		static inline std::shared_ptr<CShader> CircleShader = nullptr;

		struct FCameraData
		{
			glm::mat4 ViewProjection = glm::mat4(1.0f);
		} static inline CameraData;
		static inline std::unique_ptr<CUniformBuffer> CameraUniformBuffer = nullptr;
	};

}