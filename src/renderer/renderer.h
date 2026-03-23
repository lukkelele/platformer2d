#pragma once

#include <utility>

#include <glm/glm.hpp>

#include "core/core.h"
#include "core/log_formatters.h"
#include "backendinfo.h"
#include "camera.h"
#include "color.h"
#include "framebuffer.h"
#include "rendercommandqueue.h"
#include "shader.h"
#include "sprite.h"
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
		float OutlineThickness = 0.0f;
		glm::vec4 OutlineColor = FColor::Transparent;
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
		static void BeginScene(const glm::mat4& ViewProj, const glm::mat4& Transform);

		static void StartBatch();
		static void NextBatch();
		static void Flush();

		template<typename TRenderFunction>
		static void Submit(TRenderFunction&& Func)
		{
			auto RenderCommand = [](void* Ptr)
			{
				auto FunctionPtr = (TRenderFunction*)Ptr;
				(*FunctionPtr)();
				FunctionPtr->~TRenderFunction();
			};

			auto StorageBuffer = GetRenderCommandQueue().Allocate(RenderCommand, sizeof(Func));
			new (StorageBuffer) TRenderFunction(std::forward<TRenderFunction>(Func));
		}

		static std::shared_ptr<CFramebuffer> GetViewportFramebuffer();
		static uint16_t GetFrameIndex();

		static void DrawQuad(const glm::vec2& Pos, const glm::vec2& Size, const glm::vec4& Color, float RotationDeg = 0.0f, float OutlineThickness = 0.0f, const glm::vec4& OutlineColor = FColor::Transparent);
		static void DrawQuad(const glm::vec2& Pos, const glm::vec2& Size, const CTexture& Texture, const glm::vec4& Color = FColor::White, float RotationDeg = 0.0f, float OutlineThickness = 0.0f, const glm::vec4& OutlineColor = FColor::Transparent);
		static void DrawQuad(const glm::vec3& Pos, const glm::vec2& Size, const CTexture& Texture, const glm::vec4& Color = FColor::White, float RotationDeg = 0.0f, float OutlineThickness = 0.0f, const glm::vec4& OutlineColor = FColor::Transparent);
		static void DrawQuad(const glm::vec2& Pos, const glm::vec2& Size, const CTexture& Texture, std::span<const glm::vec2, 4> TexCoords, const glm::vec4& Color = FColor::White, float RotationDeg = 0.0f, float OutlineThickness = 0.0f, const glm::vec4& OutlineColor = FColor::Transparent);
		static void DrawQuad(const glm::vec3& Pos, const glm::vec2& Size, const CTexture& Texture, std::span<const glm::vec2, 4> TexCoords, const glm::vec4& Color = FColor::White, float RotationDeg = 0.0f, float OutlineThickness = 0.0f, const glm::vec4& OutlineColor = FColor::Transparent);
		static void DrawQuad(const glm::vec2& Pos, const glm::vec2& Size, const CTexture& Texture, const FSpriteUV& UV, const glm::vec4& Color = FColor::White, float RotationDeg = 0.0f, float OutlineThickness = 0.0f, const glm::vec4& OutlineColor = FColor::Transparent);
		static void DrawQuad(const glm::vec3& Pos, const glm::vec2& Size, const CTexture& Texture, const FSpriteUV& UV, const glm::vec4& Color = FColor::White, float RotationDeg = 0.0f, float OutlineThickness = 0.0f, const glm::vec4& OutlineColor = FColor::Transparent);
		static void DrawQuad(const glm::vec2& Pos, const glm::vec2& Size, ETexture Texture, const glm::vec4& Color = FColor::White, float RotationDeg = 0.0f, float OutlineThickness = 0.0f, const glm::vec4& OutlineColor = FColor::Transparent);

		static void DrawLine(const glm::vec2& P0, const glm::vec2& P1, const glm::vec4& Color, uint16_t LineWidth = 8);
		static void DrawLine(const glm::vec3& P0, const glm::vec3& P1, const glm::vec4& Color, uint16_t LineWidth = 8);

		static void DrawCircle(const glm::vec2& P0, const glm::vec3& Rotation, float Radius, const glm::vec4& Color);
		static void DrawCircle(const glm::vec3& P0, const glm::vec3& Rotation, float Radius, const glm::vec4& Color);
		static void DrawCircle(const glm::mat4& Transform, const glm::vec4& Color);
		static void DrawCircleFilled(const glm::vec2& P0, float Radius, const glm::vec4& Color, float Thickness = 1.0f);
		static void DrawCircleFilled(const glm::vec3& P0, float Radius, const glm::vec4& Color, float Thickness = 1.0f);

		static void DrawTransform(const glm::mat4& Transform, float Scale = 1.0f, const glm::vec4& Color = FColor::Magenta);

		static const glm::vec4& GetClearColor();
		static void SetClearColor(const glm::vec4& InClearColor);
		static void SetLineWidth(uint16_t LineWidth);
		static void SetDepthTest(bool Enabled);
		static bool GetDepthTest();
		static void SetDepthFunction(uint32_t DepthFunc);
		static uint32_t GetDepthFunction();

		static const FBackendInfo& GetBackendInfo() { return BackendInfo; }

		static const FDrawStatistics& GetDrawStatistics();
		static void ResetDrawStatistics();

		static void SetCameraViewProjection(const glm::mat4& ViewProj);

		static std::shared_ptr<CTexture> GetWhiteTexture();
		static std::shared_ptr<CTexture> GetTexture(ETexture Texture);
		static const std::unordered_map<ETexture, std::shared_ptr<CTexture>>& GetTextures();
		static std::shared_ptr<CShader> GetShader(CShader::EType ShaderType);

		static void SetBlending(bool Enabled);
		static void SetBlendFunction(uint32_t Source, uint32_t Destination);
		static uint32_t GetBlendSource();
		static uint32_t GetBlendDestination();
		static std::pair<uint32_t, uint32_t> GetBlendFunction();

		static void SetDebugRender(bool Enabled);

	private:
		static void CreateFramebuffer();
		static void SetupQuadRenderer();
		static void SetupLineRenderer();
		static void SetupCircleRenderer();
		static void LoadTextures();

		static void SwapQueues();
		static uint8_t GetRenderQueueIndex();
		static uint8_t GetRenderQueueSubmissionIndex();
		static CRenderCommandQueue& GetRenderCommandQueue();

		CRenderer& operator=(const CRenderer&) = delete;
		CRenderer& operator=(CRenderer&&) = delete;

	public:
		static constexpr int MaxTextures = 16;
		static constexpr std::array<glm::vec2, 4> TextureCoords = {
			glm::vec2(0, 0), /* Bottom left */
			glm::vec2(0, 1), /* Top left */
			glm::vec2(1, 1), /* Top right */
			glm::vec2(1, 0), /* Bottom right */
		};
		static constexpr std::array<glm::vec2, 4> MirroredTextureCoords = {
			glm::vec2(1, 0), /* Bottom left */
			glm::vec2(1, 1), /* Top left */
			glm::vec2(0, 1), /* Top right */
			glm::vec2(0, 0), /* Bottom right */
		};
	private:
		static inline bool bInitialized = false;
		static inline FBackendInfo BackendInfo;

		static inline GLuint QuadVAO = 0;
		static inline GLuint QuadVBO = 0;
		static inline GLuint QuadEBO = 0;
		static inline uint32_t QuadIndexCount = 0;
		static constexpr glm::vec4 QuadVertexPositions[4] = {
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
		struct FLineConfig
		{
			uint16_t Width = 2;
		};
		static FLineConfig LineConfig;

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
		};
		static FCameraData CameraData;
		static inline std::unique_ptr<CUniformBuffer> CameraUniformBuffer = nullptr;

		static inline bool bDebugRender = false;
	};

}