#pragma once

#include <utility>

#include <glm/glm.hpp>

#include "core/core.h"
#include "core/log_formatters.h"
#include "backendinfo.h"
#include "camera.h"
#include "color.h"
#include "font.h"
#include "fontatlas.h"
#include "framebuffer.h"
#include "rendercommandqueue.h"
#include "shader.h"
#include "sprite.h"
#include "texture.h"
#include "uniformbuffer.h"

namespace platformer2d {

	class CRenderThread;
	struct FSpriteSheet;

	struct FQuadVertex
	{
		glm::vec3 Position = {0.0f, 0.0f, 0.0f};
		glm::vec4 Color = {1.0f, 1.0f, 1.0f, 1.0f};
		glm::vec2 TexCoord = {0.0f, 0.0f};
		int TexIndex = 0;
		float TileFactor = 1.0f;
		float OutlineThickness = 0.0f;
		glm::vec4 OutlineColor = FColor::Transparent;
	};

	struct FLineVertex
	{
		glm::vec3 Position = {0.0f, 0.0f, 0.0f};
		glm::vec4 Color = {1.0f, 1.0f, 1.0f, 1.0f};
	};

	struct FCircleVertex
	{
		glm::vec3 WorldPosition = {0.0f, 0.0f, 0.0f};
		float Thickness = 1.0f;
		glm::vec2 LocalPosition = {0.0f, 0.0f};
		glm::vec4 Color = {1.0f, 1.0f, 1.0f, 1.0f};
	};

	struct FTextVertex
	{
		glm::vec3 Position = {0.0f, 0.0f, 0.0f};
		glm::vec4 Color = {1.0f, 1.0f, 1.0f, 1.0f};
		glm::vec2 TexCoord = {0.0f, 0.0f};
		glm::vec4 OutlineColor = {0.0f, 0.0f, 0.0f, 0.0f};
		float OutlineWidth = 0.0f;
	};

	struct FDrawStatistics
	{
		std::uint64_t QuadCount = 0;
		std::uint64_t LineCount = 0;
		std::uint64_t GlyphCount = 0;
	};

	class CRenderer
	{
	public:
		CRenderer() = delete;
		CRenderer(const CRenderer&) = delete;
		CRenderer(CRenderer&&) = delete;
		~CRenderer() = delete;

		CRenderer& operator=(const CRenderer&) = delete;
		CRenderer& operator=(CRenderer&&) = delete;

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

		template<typename TFunc>
		static void Submit(TFunc&& Func)
		{
			auto Cmd = [](void* Ptr)
			{
				auto CmdPtr = (TFunc*)Ptr;
				(*CmdPtr)();
				std::destroy_at(CmdPtr);
			};

			auto* Buffer = GetRenderCommandQueue().Allocate(Cmd, sizeof(Func));
			new (Buffer) TFunc(std::forward<TFunc>(Func));
		}

		[[nodiscard]] static std::shared_ptr<CFramebuffer> GetViewportFramebuffer();
		[[nodiscard]] static std::uint16_t GetFrameIndex();

		static void DrawQuad(const glm::vec2& Pos, const glm::vec2& Size, const glm::vec4& Color, float RotationDeg = 0.0f, float OutlineThickness = 0.0f, const glm::vec4& OutlineColor = FColor::Transparent);
		static void DrawQuad(const glm::vec2& Pos, const glm::vec2& Size, const CTexture& Texture, const glm::vec4& Color = FColor::White, float RotationDeg = 0.0f, float OutlineThickness = 0.0f, const glm::vec4& OutlineColor = FColor::Transparent);
		static void DrawQuad(const glm::vec3& Pos, const glm::vec2& Size, const CTexture& Texture, const glm::vec4& Color = FColor::White, float RotationDeg = 0.0f, float OutlineThickness = 0.0f, const glm::vec4& OutlineColor = FColor::Transparent);
		static void DrawQuad(const glm::vec2& Pos, const glm::vec2& Size, const CTexture& Texture, std::span<const glm::vec2, 4> TexCoords, const glm::vec4& Color = FColor::White, float RotationDeg = 0.0f, float OutlineThickness = 0.0f, const glm::vec4& OutlineColor = FColor::Transparent);
		static void DrawQuad(const glm::vec3& Pos, const glm::vec2& Size, const CTexture& Texture, std::span<const glm::vec2, 4> TexCoords, const glm::vec4& Color = FColor::White, float RotationDeg = 0.0f, float OutlineThickness = 0.0f, const glm::vec4& OutlineColor = FColor::Transparent);
		static void DrawQuad(const glm::vec2& Pos, const glm::vec2& Size, const CTexture& Texture, const FSpriteUV& UV, const glm::vec4& Color = FColor::White, float RotationDeg = 0.0f, float OutlineThickness = 0.0f, const glm::vec4& OutlineColor = FColor::Transparent);
		static void DrawQuad(const glm::vec3& Pos, const glm::vec2& Size, const CTexture& Texture, const FSpriteUV& UV, const glm::vec4& Color = FColor::White, float RotationDeg = 0.0f, float OutlineThickness = 0.0f, const glm::vec4& OutlineColor = FColor::Transparent);
		static void DrawQuad(const glm::vec2& Pos, const glm::vec2& Size, ETexture Texture, const glm::vec4& Color = FColor::White, float RotationDeg = 0.0f, float OutlineThickness = 0.0f, const glm::vec4& OutlineColor = FColor::Transparent);

		static void DrawLine(const glm::vec2& P0, const glm::vec2& P1, const glm::vec4& Color, std::uint16_t LineWidth = 8);
		static void DrawLine(const glm::vec3& P0, const glm::vec3& P1, const glm::vec4& Color, std::uint16_t LineWidth = 8);

		static void DrawCircle(const glm::vec2& P0, const glm::vec3& Rotation, float Radius, const glm::vec4& Color);
		static void DrawCircle(const glm::vec3& P0, const glm::vec3& Rotation, float Radius, const glm::vec4& Color);
		static void DrawCircle(const glm::mat4& Transform, const glm::vec4& Color);
		static void DrawCircleFilled(const glm::vec2& P0, float Radius, const glm::vec4& Color, float Thickness = 1.0f);
		static void DrawCircleFilled(const glm::vec3& P0, float Radius, const glm::vec4& Color, float Thickness = 1.0f);

		static void DrawTransform(const glm::mat4& Transform, float Scale = 1.0f, const glm::vec4& Color = FColor::Magenta);
		static void DrawCrossMark(const glm::vec2& Pos, const glm::vec4& Color = FColor::Black, std::uint16_t LineWidth = 2, float CrossArm = 0.08f, float MarkerRadius = 0.020f);
		static void DrawCrossMark(const glm::vec3& Pos, const glm::vec4& Color = FColor::Black, std::uint16_t LineWidth = 2, float CrossArm = 0.08f, float MarkerRadius = 0.020f);

		static void DrawText(const CFontAtlas& Font, std::string_view Text, const glm::vec3& Pos, float Scale = 0.30f, const glm::vec4& Color = FColor::White, const glm::vec4& OutlineColor = FColor::Transparent, float OutlineWidth = 0.0f);
		static void DrawText(std::string_view Text, const glm::vec3& Pos, float Scale = 0.30f, const glm::vec4& Color = FColor::White, const glm::vec4& OutlineColor = FColor::Transparent, float OutlineWidth = 0.0f);
		static void DrawText(EFont Font, std::string_view Text, const glm::vec2& Pos, float Scale = 0.30f, const glm::vec4& Color = FColor::White, const glm::vec4& OutlineColor = FColor::Transparent, float OutlineWidth = 0.0f);
		static void DrawText(EFont Font, std::string_view Text, const glm::vec3& Pos, float Scale = 0.30f, const glm::vec4& Color = FColor::White, const glm::vec4& OutlineColor = FColor::Transparent, float OutlineWidth = 0.0f);
		static void DrawText(EFont Font, EFontModifier FontMod, std::string_view Text, const glm::vec2& Pos, float Scale = 0.30f, const glm::vec4& Color = FColor::White, const glm::vec4& OutlineColor = FColor::Transparent, float OutlineWidth = 0.0f);
		static void DrawText(EFont Font, EFontModifier FontMod, std::string_view Text, const glm::vec3& Pos, float Scale = 0.30f, const glm::vec4& Color = FColor::White, const glm::vec4& OutlineColor = FColor::Transparent, float OutlineWidth = 0.0f);
		static void DrawTextScreen(const CFontAtlas& Font, std::string_view Text, const glm::vec2& PixelPos, float PixelSize, const glm::vec4& Color = FColor::White, const glm::vec4& OutlineColor = FColor::Transparent, float OutlineWidth = 0.0f);
		static void DrawTextScreen(std::string_view Text, const glm::vec2& PixelPos, float PixelSize, const glm::vec4& Color = FColor::White, const glm::vec4& OutlineColor = FColor::Transparent, float OutlineWidth = 0.0f);
		static void DrawTextScreen(EFont Font, std::string_view Text, const glm::vec2& PixelPos, float PixelSize, const glm::vec4& Color = FColor::White, const glm::vec4& OutlineColor = FColor::Transparent, float OutlineWidth = 0.0f);
		static void DrawTextScreen(EFont Font, EFontModifier FontMod, std::string_view Text, const glm::vec2& PixelPos, float PixelSize, const glm::vec4& Color = FColor::White, const glm::vec4& OutlineColor = FColor::Transparent, float OutlineWidth = 0.0f);

		[[nodiscard]] static const CFontAtlas& GetFont(EFont Font, EFontModifier Modifier = EFontModifier::Normal);
		[[nodiscard]] static const CFontAtlas& GetDefaultFont();

		[[nodiscard]] static const glm::vec4& GetClearColor();
		static void SetClearColor(const glm::vec4& InClearColor);
		static void SetLineWidth(std::uint16_t LineWidth);
		static void SetDepthTest(bool Enabled);
		[[nodiscard]] static bool GetDepthTest();
		static void SetDepthFunction(std::uint32_t DepthFunc);
		[[nodiscard]] static std::uint32_t GetDepthFunction();

		[[nodiscard]] static const FBackendInfo& GetBackendInfo() { return BackendInfo; }
		[[nodiscard]] static const FDrawStatistics& GetDrawStatistics();
		static void ResetDrawStatistics();

		static void SetCameraViewProjection(const glm::mat4& ViewProj);

		[[nodiscard]] static std::shared_ptr<CTexture> GetWhiteTexture();
		[[nodiscard]] static std::shared_ptr<CTexture> GetTexture(ETexture Texture);
		[[nodiscard]] static ETexture GetTexture(const std::filesystem::path& Path);
		[[nodiscard]] static const std::map<ETexture, std::shared_ptr<CTexture>>& GetTextures();
		[[nodiscard]] static const FSpriteSheet* GetSpriteSheet(ETexture Texture);
		[[nodiscard]] static FSpriteSheet* GetSpriteSheetMutable(ETexture Texture);
		[[nodiscard]] static std::shared_ptr<CShader> GetShader(CShader::EType ShaderType);
		[[nodiscard]] static const std::span<const glm::vec2, 4> GetTextureCoords(EDirection Dir);

		static void SetBlending(bool Enabled);
		static void SetBlendFunction(std::uint32_t Source, std::uint32_t Destination);
		[[nodiscard]] static std::uint32_t GetBlendSource();
		[[nodiscard]] static std::uint32_t GetBlendDestination();
		[[nodiscard]] static std::pair<std::uint32_t, std::uint32_t> GetBlendFunction();

		static void SetDebugRender(bool Enabled);

	private:
		static void CreateFramebuffer();
		static void SetupQuadRenderer();
		static void SetupLineRenderer();
		static void SetupCircleRenderer();
		static void SetupTextRenderer();
		static void LoadTextures();
		static void LoadFonts();

		static void SwapQueues();
		[[nodiscard]] static std::uint8_t GetRenderQueueIndex();
		[[nodiscard]] static std::uint8_t GetRenderQueueSubmissionIndex();
		[[nodiscard]] static CRenderCommandQueue& GetRenderCommandQueue();

	public:
		static constexpr int MaxTextures = 16;
		static constexpr std::array<glm::vec2, 4> TextureCoords = {
			glm::vec2(0, 0), /* BL */
			glm::vec2(0, 1), /* TL */
			glm::vec2(1, 1), /* TR */
			glm::vec2(1, 0), /* BR */
		};
		static constexpr std::array<glm::vec2, 4> MirroredTextureCoords = {
			glm::vec2(1, 0), /* BL */
			glm::vec2(1, 1), /* TL */
			glm::vec2(0, 1), /* TR */
			glm::vec2(0, 0), /* BR */
		};

	private:
		static inline bool bInitialized = false;
		static inline FBackendInfo BackendInfo;

		static inline GLuint QuadVAO = 0;
		static inline GLuint QuadVBO = 0;
		static inline GLuint QuadEBO = 0;
		static inline std::uint32_t QuadIndexCount = 0;
		static constexpr glm::vec4 QuadVertexPositions[4] = {
			{-0.50f, -0.50f, 0.0f, 1.0f},
			{-0.50f,  0.50f, 0.0f, 1.0f},
			{ 0.50f,  0.50f, 0.0f, 1.0f},
			{ 0.50f, -0.50f, 0.0f, 1.0f}
        };
		static inline FQuadVertex* QuadVertexBufferBase = nullptr;
		static inline FQuadVertex* QuadVertexBufferPtr = nullptr;
		static inline std::shared_ptr<CShader> QuadShader = nullptr;

		static inline GLuint LineVAO = 0;
		static inline GLuint LineVBO = 0;
		static inline GLuint LineEBO = 0;
		static inline std::uint32_t LineIndexCount = 0;
		static inline FLineVertex* LineVertexBufferBase = nullptr;
		static inline FLineVertex* LineVertexBufferPtr = nullptr;
		static inline std::shared_ptr<CShader> LineShader = nullptr;
		struct FLineConfig
		{
			std::uint16_t Width = 2;
		};
		static FLineConfig LineConfig;

		static inline GLuint CircleVAO = 0;
		static inline GLuint CircleVBO = 0;
		static inline GLuint CircleEBO = 0;
		static inline std::uint32_t CircleIndexCount = 0;
		static inline FCircleVertex* CircleVertexBufferBase = nullptr;
		static inline FCircleVertex* CircleVertexBufferPtr = nullptr;
		static inline std::shared_ptr<CShader> CircleShader = nullptr;

		struct FCameraData
		{
			glm::mat4 ViewProjection = glm::mat4(1.0f);
		};
		static FCameraData CameraData;
		static inline std::unique_ptr<CUniformBuffer> CameraUniformBuffer = nullptr;

		static inline GLuint TextWorldVAO = 0;
		static inline GLuint TextWorldVBO = 0;
		static inline std::uint32_t TextWorldIndexCount = 0;
		static inline FTextVertex* TextWorldVertexBufferBase = nullptr;
		static inline FTextVertex* TextWorldVertexBufferPtr = nullptr;
		static inline const CFontAtlas* TextWorldFont = nullptr;

		static inline GLuint TextScreenVAO = 0;
		static inline GLuint TextScreenVBO = 0;
		static inline std::uint32_t TextScreenIndexCount = 0;
		static inline FTextVertex* TextScreenVertexBufferBase = nullptr;
		static inline FTextVertex* TextScreenVertexBufferPtr = nullptr;
		static inline const CFontAtlas* TextScreenFont = nullptr;

		static inline std::shared_ptr<CShader> TextShader = nullptr;
		static inline std::shared_ptr<CFontAtlas> DefaultFont = nullptr;
		using TFontAtlasMap = std::array<std::shared_ptr<CFontAtlas>, static_cast<std::size_t>(EFontModifier::COUNT)>;
		static inline std::array<TFontAtlasMap, static_cast<std::size_t>(EFont::COUNT)> Fonts{};

		static constexpr int FontAtlasTextureSlot = 16;

		static inline bool bDebugRender = false;
	};

}
