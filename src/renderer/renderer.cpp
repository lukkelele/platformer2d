#include "renderer.h"

#include <algorithm>
#include <array>
#include <atomic>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_common.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/settings.h"
#include "core/window.h"
#include "core/profiler.h"
#include "core/timer.h"
#include "backendinfo.h"
#include "debugrenderer.h"
#include "opengl.h"
#include "rendercommandqueue.h"
#include "ui/ui.h"
#include "asset/assetmanager.h"
#include "scene/effectmanager.h"

namespace platformer2d {
	static constexpr int CIRCLE_SEGMENTS = 32;
	static constexpr std::uint32_t MAX_QUADS = 10000;
	static constexpr std::uint32_t MAX_LINES = 1000;
	static constexpr std::uint32_t MAX_VERTICES = MAX_QUADS * 4;
	static constexpr std::uint32_t MAX_INDICES = MAX_QUADS * 6;
	static constexpr std::uint32_t MAX_LINE_VERTICES = MAX_LINES * 2;
	static constexpr std::uint32_t MAX_LINE_INDICES = MAX_LINES * 2;

	CRenderer::FLineConfig CRenderer::LineConfig;
	CRenderer::FCameraData CRenderer::CameraData;

	struct FRendererData
	{
		uint16_t FrameIndex = 0;
		uint16_t RefreshRate = 0;
		std::shared_ptr<CTexture> WhiteTexture = nullptr;
		std::shared_ptr<CFramebuffer> ViewportFramebuffer = nullptr;
		std::unordered_map<ETexture, std::shared_ptr<CTexture>> Textures;

		struct
		{
			bool bBlending = false;
			uint32_t BlendSource = 0;
			uint32_t BlendDestination = 0;
			bool bDepthTest = false;
			uint32_t DepthFunc = 0;
		} GL;
	};

	namespace {

		FRendererData Data{};
		FDrawStatistics DrawStats;

		std::array<CRenderCommandQueue*, 2> CommandQueue;
		std::atomic_uint8_t CommandQueueSubmissionIndex = 0;
		const std::size_t CommandQueueCount = CommandQueue.size();

		constexpr glm::vec2 QuadTextureCoords[] = {
			{0.0f, 0.0f}, /*  Bottom Left.  */
			{0.0f, 1.0f}, /*  Top Left.     */
			{1.0f, 1.0f}, /*  Top Right.    */
			{1.0f, 0.0f}  /*  Bottom Right. */
		};
	}

	FORCEINLINE static void BindTextures()
	{
		for (auto& [Texture, TextureRef] : Data.Textures) {
			if (TextureRef != nullptr) {
				TextureRef->Bind(static_cast<std::uint32_t>(Texture));
			}
		}
	}

	void CRenderer::Initialize()
	{
		LK_VERIFY(bInitialized == false, "Initialize called multiple times");
		const int GladInitResult = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		LK_VERIFY(GladInitResult != 0, "Failed to initialize GLAD");
		LK_OpenGL_Verify(glEnable(GL_BLEND));
		SetBlendFunction(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		SetDepthTest(true);
		SetDepthFunction(GL_LESS);
		LK_OpenGL_Verify(glEnable(GL_LINE_SMOOTH));

		OpenGL::LoadInfo(BackendInfo);
		LK_INFO("OpenGL {}.{}", BackendInfo.Version.Major, BackendInfo.Version.Minor);
#ifdef LK_BUILD_DEBUG
		OpenGL::Internal::SetupDebugContext(nullptr);
#endif

		for (std::size_t Idx = 0; Idx < CommandQueue.size(); Idx++) {
			CommandQueue[Idx] = new CRenderCommandQueue();
		}

		LoadTextures();
		LK_INFO_TAG("Renderer", "Loaded {} textures", Data.Textures.size());

		CreateFramebuffer();
		SetupQuadRenderer();
		SetupLineRenderer();
		SetupCircleRenderer();

		QuadShader->Bind();
		BindTextures();

		Data.RefreshRate = CWindow::Get().GetRefreshRate();
		LK_VERIFY(Data.RefreshRate > 0, "Failed to get window refresh rate");

		CDebugRenderer::Initialize();
#ifdef LK_BUILD_DEBUG
		bDebugRender = true;
#endif

		CAssetManager::Get().Initialize();
		CEffectManager::Get().Initialize();

		/**
		 * Disable depth testing when pause menu is opened.
		 * Done to make sure the dark overlay is drawn on top the scene.
		 */
		UI::OnPauseMenuOpened.Add([](const bool Open)
		{
			if (Open) {
				SetDepthTest(false);
			} else {
				SetDepthTest(true);
			}
		});

		bInitialized = true;
	}

	void CRenderer::Destroy()
	{
		Data.WhiteTexture = nullptr;
		LK_DEBUG_TAG("Renderer", "Releasing {} textures", Data.Textures.size());
		for (auto& [Texture, TextureRef] : Data.Textures) {
			if (TextureRef != nullptr) {
				LK_TRACE_TAG("Renderer", "Release: {}", Enum::ToString(Texture));
				TextureRef->Unbind();
				TextureRef.reset();
			}
		}

		if (CameraUniformBuffer) {
			LK_DEBUG_TAG("Renderer", "Releasing uniform buffers");
			CameraUniformBuffer->Destroy();
			CameraUniformBuffer.reset();
		}

		if (Data.ViewportFramebuffer) {
			LK_DEBUG_TAG("Renderer", "Releasing viewport framebuffer");
			Data.ViewportFramebuffer->Destroy();
			Data.ViewportFramebuffer.reset();
		}
	}

	void CRenderer::CreateFramebuffer()
	{
		FFramebufferSpecification Spec;
		Spec.Attachments = {EImageFormat::RGBA32F, EImageFormat::DEPTH24STENCIL8};
		Spec.Samples = 1;
		Spec.ClearColorOnLoad = false;
		Spec.ClearColor = FColor::Convert(RGBA32::DarkerGray);
		Spec.Name = "fb-viewport";
		Spec.Width = CWindow::Get().GetWidth();
		Spec.Height = CWindow::Get().GetHeight();
		Data.ViewportFramebuffer = std::make_shared<CFramebuffer>(Spec);

		CWindow::OnFramebufferResized.Add([&](const uint32_t NewWidth, const uint32_t NewHeight)
		{
			if ((NewWidth <= 0) || (NewHeight <= 0)) {
				return;
			}
			LK_TRACE_TAG("Renderer", "OnFramebufferResized: ({}, {})", NewWidth, NewHeight);
			Data.ViewportFramebuffer->Resize(NewWidth, NewHeight);
		});
	}

	void CRenderer::SetupQuadRenderer()
	{
		const FVertexBufferLayout QuadLayout = {
			/* clang-format off */
			{ "pos",              EShaderDataType::Float3, },
			{ "color",            EShaderDataType::Float4, },
			{ "texcoord",         EShaderDataType::Float2, },
			{ "texindex",         EShaderDataType::Int,    },
			{ "tilefactor",       EShaderDataType::Float,  },
			{ "outlinethickness", EShaderDataType::Float,  },
			{ "outlinecolor",     EShaderDataType::Float4, },
			/* clang-format on */
		};

		QuadVAO = OpenGL::VertexArray::Create();
		QuadVBO = OpenGL::VertexBuffer::Create(MAX_VERTICES * sizeof(FQuadVertex), QuadLayout);

		QuadVertexBufferBase = new FQuadVertex[MAX_VERTICES];
		QuadVertexBufferPtr = QuadVertexBufferBase;

		std::uint32_t* QuadIndices = new std::uint32_t[MAX_INDICES];
		std::uint32_t Offset = 0;
		for (std::uint32_t Idx = 0; Idx < MAX_INDICES; Idx += 6) {
			/* First triangle, 0->1->2 */
			QuadIndices[Idx + 0] = Offset + 0;
			QuadIndices[Idx + 1] = Offset + 1;
			QuadIndices[Idx + 2] = Offset + 2;

			/* Second triangle, 2->3->0 */
			QuadIndices[Idx + 3] = Offset + 2;
			QuadIndices[Idx + 4] = Offset + 3;
			QuadIndices[Idx + 5] = Offset + 0;

			Offset += 4;
		}

		QuadEBO = OpenGL::ElementBuffer::Create(QuadIndices, MAX_INDICES * sizeof(uint32_t));
		delete[] QuadIndices;

		QuadShader = std::make_shared<CShader>(SHADERS_DIR "/quad.shader");
		/* Set every texture binding. */
		for (auto& [Texture, TextureRef] : Data.Textures) {
			LK_VERIFY(TextureRef, "Invalid texture reference: {}", Enum::ToString(Texture));
			const int Idx = static_cast<int>(Texture);
			QuadShader->Set(Format("u_texture{}", Idx), Idx);
			TextureRef->Bind(Idx);
			TextureRef->SetSlot(Idx);
		}

		CameraData.ViewProjection = glm::mat4(1.0f);
		CameraUniformBuffer = std::make_unique<CUniformBuffer>(sizeof(FCameraData), "CameraUB");
		CameraUniformBuffer->SetBinding(QuadShader, "ub_camera", 0);
		CameraUniformBuffer->SetData(&CameraData, sizeof(FCameraData));
	}

	void CRenderer::SetupLineRenderer()
	{
		const FVertexBufferLayout LineLayout = {
			/* clang-format off */
			{ "pos",   EShaderDataType::Float3, },
			{ "color", EShaderDataType::Float4, },
			/* clang-format on */
		};

		LineVAO = OpenGL::VertexArray::Create();
		LineVBO = OpenGL::VertexBuffer::Create(MAX_LINE_VERTICES * sizeof(FLineVertex), LineLayout);
		LineVertexBufferBase = new FLineVertex[MAX_LINE_VERTICES];
		LineVertexBufferPtr = LineVertexBufferBase;

		std::uint32_t* LineIndices = new std::uint32_t[MAX_LINE_INDICES];
		for (std::uint32_t Idx = 0; Idx < MAX_LINE_INDICES; Idx++) {
			LineIndices[Idx] = Idx;
		}
		LineEBO = OpenGL::ElementBuffer::Create(LineIndices, MAX_LINE_INDICES * sizeof(uint32_t));
		delete[] LineIndices;

		LineShader = std::make_shared<CShader>(SHADERS_DIR "/line.shader");
		LK_OpenGL_Verify(glLineWidth(LineConfig.Width));
	}

	void CRenderer::SetupCircleRenderer()
	{
		const FVertexBufferLayout CircleLayout = {
			/* clang-format off */
			{ "worldpos",  EShaderDataType::Float3, },
			{ "thickness", EShaderDataType::Float,  },
			{ "localpos",  EShaderDataType::Float2, },
			{ "color",     EShaderDataType::Float4, },
			/* clang-format on */
		};

		CircleVAO = OpenGL::VertexArray::Create();
		CircleVBO = OpenGL::VertexBuffer::Create(MAX_VERTICES * sizeof(FCircleVertex), CircleLayout);

		CircleVertexBufferBase = new FCircleVertex[MAX_VERTICES];
		CircleVertexBufferPtr = CircleVertexBufferBase;

		/**
		 * Re-use the quad EBO as the rendering of filled circles use
		 * triangles in segments.
		 */
		LK_OpenGL_Verify(glBindVertexArray(CircleVAO));
		LK_OpenGL_Verify(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, QuadEBO));
		LK_OpenGL_Verify(glBindVertexArray(0));

		CircleShader = std::make_shared<CShader>(SHADERS_DIR "/circle.shader");
	}

	void CRenderer::LoadTextures()
	{
		Data.Textures.reserve(MaxTextures);

		auto LoadTexture = [](std::string_view Path, const ETexture Texture,
							   const EImageFormat Format = EImageFormat::RGBA8,
							   const glm::vec2& Size = {0.0f, 0.0f}) -> void
		{
			LK_VERIFY(std::filesystem::exists(Path), "Texture {} does not exist", static_cast<int>(Texture));
			LK_VERIFY(!Data.Textures.contains(Texture));
			FTextureSpecification Spec = {
				.Path = Path.data(),
				.Format = Format,
				.SamplerWrap = ETextureWrap::Clamp,
				.SamplerFilter = ETextureFilter::Nearest,
			};
			if (Size.x > 0.0f) {
				Spec.Width = Size.x;
			}
			if (Size.y > 0.0f) {
				Spec.Height = Size.y;
			}

			Data.Textures.emplace(std::make_pair(Texture, std::make_shared<CTexture>(Spec)));
		};

		LoadTexture(TEXTURES_DIR "/white.png", ETexture::White, EImageFormat::RGBA8, {1.0f, 1.0f});
		LoadTexture(TEXTURES_DIR "/sunny.png", ETexture::Background, EImageFormat::RGBA8);
		LoadTexture(TEXTURES_DIR "/characters.png", ETexture::Player, EImageFormat::RGBA8);
		LoadTexture(TEXTURES_DIR "/metal.png", ETexture::Metal, EImageFormat::RGBA8);
		LoadTexture(TEXTURES_DIR "/bricks.png", ETexture::Bricks, EImageFormat::RGBA8);
		LoadTexture(TEXTURES_DIR "/wood.png", ETexture::Wood, EImageFormat::RGBA8);
		LoadTexture(TEXTURES_DIR "/swoosh.png", ETexture::Swoosh, EImageFormat::RGBA8);
		LoadTexture(TEXTURES_DIR "/cloud-1.png", ETexture::Cloud, EImageFormat::RGBA8);
		LoadTexture(TEXTURES_DIR "/ar15.png", ETexture::Rifle, EImageFormat::RGBA8);
		LoadTexture(TEXTURES_DIR "/goblin.png", ETexture::Goblin, EImageFormat::RGBA8);
		Data.WhiteTexture = Data.Textures[ETexture::White];
	}

	void CRenderer::SwapQueues()
	{
		CommandQueueSubmissionIndex = (CommandQueueSubmissionIndex + 1) % CommandQueueCount;
	}

	std::uint8_t CRenderer::GetRenderQueueIndex()
	{
		return (CommandQueueSubmissionIndex + 1) % CommandQueueCount;
	}

	std::uint8_t CRenderer::GetRenderQueueSubmissionIndex()
	{
		return CommandQueueSubmissionIndex;
	}

	CRenderCommandQueue& CRenderer::GetRenderCommandQueue()
	{
		return *CommandQueue[CommandQueueSubmissionIndex];
	}

	void CRenderer::BeginFrame()
	{
		LK_PROFILE_FUNC();
		Data.FrameIndex = (Data.FrameIndex + 1) % Data.RefreshRate;
		SwapQueues();

		CFramebuffer::ClearDefault();
		Data.ViewportFramebuffer->Clear();

		QuadShader->Bind();
		BindTextures();
	}

	void CRenderer::EndFrame()
	{
		LK_PROFILE_FUNC();
		CommandQueue[GetRenderQueueIndex()]->Execute();
		Flush();
	}

	void CRenderer::BeginScene(const CCamera& Camera)
	{
		LK_PROFILE_FUNC();
		CameraData.ViewProjection = Camera.GetViewProjection();
		CameraUniformBuffer->SetData(&CameraData, sizeof(FCameraData));

		if (bDebugRender) {
			CDebugRenderer::ViewProjection = CameraData.ViewProjection;
		}

		StartBatch();
	}

	void CRenderer::BeginScene(const CCamera& Camera, const glm::mat4& Transform)
	{
		LK_PROFILE_FUNC();
		CameraData.ViewProjection = Camera.GetViewProjection() * glm::inverse(Transform);
		CameraUniformBuffer->SetData(&CameraData, sizeof(FCameraData));
		StartBatch();
	}

	void CRenderer::BeginScene(const glm::mat4& ViewProj, const glm::mat4& Transform)
	{
		LK_PROFILE_FUNC();
		CameraData.ViewProjection = ViewProj * glm::inverse(Transform);
		CameraUniformBuffer->SetData(&CameraData, sizeof(FCameraData));
		StartBatch();
	}

	void CRenderer::StartBatch()
	{
		LK_PROFILE_FUNC();
		QuadIndexCount = 0;
		QuadVertexBufferPtr = QuadVertexBufferBase;

		LineIndexCount = 0;
		LineVertexBufferPtr = LineVertexBufferBase;

		CircleIndexCount = 0;
		CircleVertexBufferPtr = CircleVertexBufferBase;
	}

	void CRenderer::NextBatch()
	{
		LK_PROFILE_FUNC();
		Flush();
		StartBatch();
	}

	void CRenderer::Flush()
	{
		LK_PROFILE_FUNC();
		Data.ViewportFramebuffer->Bind();

		const float Brightness = std::clamp(FSettings::Get().Graphics.Brightness, 0.0f, 4.0f);

		if (QuadIndexCount > 0) {
			const std::uint32_t DataSize = static_cast<std::uint32_t>((std::uint8_t*)QuadVertexBufferPtr - (std::uint8_t*)QuadVertexBufferBase);
			LK_OpenGL_Verify(glBindBuffer(GL_ARRAY_BUFFER, QuadVBO));
			LK_OpenGL_Verify(glBufferSubData(GL_ARRAY_BUFFER, 0, DataSize, QuadVertexBufferBase));

			QuadShader->Bind();
			QuadShader->Set("u_brightness", Brightness);
			CameraUniformBuffer->Bind();
			LK_OpenGL_Verify(glBindVertexArray(QuadVAO));
			LK_OpenGL_Verify(glDrawElements(GL_TRIANGLES, QuadIndexCount, GL_UNSIGNED_INT, nullptr));
			CameraUniformBuffer->Unbind();
			QuadShader->Unbind();
		}

		if (LineIndexCount > 0) {
			const std::uint32_t DataSize = static_cast<std::uint32_t>((std::uint8_t*)LineVertexBufferPtr - (std::uint8_t*)LineVertexBufferBase);
			LK_OpenGL_Verify(glBindBuffer(GL_ARRAY_BUFFER, LineVBO));
			LK_OpenGL_Verify(glBufferSubData(GL_ARRAY_BUFFER, 0, DataSize, LineVertexBufferBase));

			LineShader->Bind();
			LineShader->Set("u_brightness", Brightness);
			CameraUniformBuffer->Bind();
			LK_OpenGL_Verify(glBindVertexArray(LineVAO));
			LK_OpenGL_Verify(glDrawElements(GL_LINES, LineIndexCount, GL_UNSIGNED_INT, nullptr));
			CameraUniformBuffer->Unbind();
			LineShader->Unbind();
		}

		if (CircleIndexCount > 0) {
			const std::uint32_t DataSize = static_cast<std::uint32_t>((std::uint8_t*)CircleVertexBufferPtr - (std::uint8_t*)CircleVertexBufferBase);
			LK_OpenGL_Verify(glBindBuffer(GL_ARRAY_BUFFER, CircleVBO));
			LK_OpenGL_Verify(glBufferSubData(GL_ARRAY_BUFFER, 0, DataSize, CircleVertexBufferBase));

			CircleShader->Bind();
			CircleShader->Set("u_brightness", Brightness);
			CameraUniformBuffer->Bind();
			LK_OpenGL_Verify(glBindVertexArray(CircleVAO));
			LK_OpenGL_Verify(glDrawElements(GL_TRIANGLES, CircleIndexCount, GL_UNSIGNED_INT, nullptr));
			CameraUniformBuffer->Unbind();
			CircleShader->Unbind();
		}

		Data.ViewportFramebuffer->Unbind();
	}

	std::shared_ptr<CFramebuffer> CRenderer::GetViewportFramebuffer()
	{
		return Data.ViewportFramebuffer;
	}

	std::uint16_t CRenderer::GetFrameIndex()
	{
		return Data.FrameIndex;
	}

	void CRenderer::DrawQuad(const glm::vec2& Pos, const glm::vec2& Size, const glm::vec4& Color, const float RotationDeg, const float OutlineThickness, const glm::vec4& OutlineColor)
	{
		if (QuadIndexCount >= MAX_INDICES) {
			NextBatch();
		}

		constexpr int TextureIndex = 0;
		constexpr float TileFactor = 1.0f;

		const glm::mat4 Transform = glm::translate(glm::mat4(1.0f), {Pos.x, Pos.y, 0.0f})
			* glm::rotate(glm::mat4(1.0f), glm::radians(RotationDeg), glm::vec3(0.0f, 0.0f, 1.0f))
			* glm::scale(glm::mat4(1.0f), {Size.x, Size.y, 1.0f});

		for (std::size_t Idx = 0; Idx < 4; Idx++) {
			QuadVertexBufferPtr->Position = Transform * QuadVertexPositions[Idx];
			QuadVertexBufferPtr->Color = Color;
			QuadVertexBufferPtr->TexCoord = QuadTextureCoords[Idx];
			QuadVertexBufferPtr->TexIndex = TextureIndex;
			QuadVertexBufferPtr->TileFactor = TileFactor;
			QuadVertexBufferPtr->OutlineThickness = OutlineThickness;
			QuadVertexBufferPtr->OutlineColor = OutlineColor;
			QuadVertexBufferPtr++;
		}

		QuadIndexCount += 6;
	}

	void CRenderer::DrawQuad(const glm::vec2& Pos, const glm::vec2& Size, const CTexture& Texture, const glm::vec4& Color,
		const float RotationDeg, const float OutlineThickness, const glm::vec4& OutlineColor)
	{
		DrawQuad({Pos.x, Pos.y, 0.010f}, Size, Texture, Color, RotationDeg, OutlineThickness, OutlineColor);
	}

	void CRenderer::DrawQuad(const glm::vec3& Pos, const glm::vec2& Size, const CTexture& Texture, const glm::vec4& Color,
		const float RotationDeg, const float OutlineThickness, const glm::vec4& OutlineColor)
	{
		if (QuadIndexCount >= MAX_INDICES) {
			NextBatch();
		}

		constexpr float TileFactor = 1.0f;

		const glm::mat4 Transform = glm::translate(glm::mat4(1.0f), Pos)
			* glm::rotate(glm::mat4(1.0f), glm::radians(RotationDeg), glm::vec3(0.0f, 0.0f, 1.0f))
			* glm::scale(glm::mat4(1.0f), {Size.x, Size.y, 1.0f});

		for (std::size_t Idx = 0; Idx < 4; Idx++) {
			QuadVertexBufferPtr->Position = Transform * QuadVertexPositions[Idx];
			QuadVertexBufferPtr->Color = Color;
			QuadVertexBufferPtr->TexCoord = QuadTextureCoords[Idx];
			QuadVertexBufferPtr->TexIndex = Texture.GetSlot();
			QuadVertexBufferPtr->TileFactor = TileFactor;
			QuadVertexBufferPtr->OutlineThickness = OutlineThickness;
			QuadVertexBufferPtr->OutlineColor = OutlineColor;
			QuadVertexBufferPtr++;
		}

		QuadIndexCount += 6;
		DrawStats.QuadCount++;
	}

	void CRenderer::DrawQuad(const glm::vec2& Pos, const glm::vec2& Size, const CTexture& Texture, std::span<const glm::vec2, 4> TexCoords,
		const glm::vec4& Color, const float RotationDeg, const float OutlineThickness, const glm::vec4& OutlineColor)
	{
		DrawQuad({Pos.x, Pos.y, 0.0f}, Size, Texture, TexCoords, Color, RotationDeg, OutlineThickness, OutlineColor);
	}

	void CRenderer::DrawQuad(const glm::vec3& Pos, const glm::vec2& Size, const CTexture& Texture, std::span<const glm::vec2, 4> TexCoords,
		const glm::vec4& Color, const float RotationDeg, const float OutlineThickness, const glm::vec4& OutlineColor)
	{
		if (QuadIndexCount >= MAX_INDICES) {
			NextBatch();
		}

		constexpr float TileFactor = 1.0f;

		const glm::mat4 Transform = glm::translate(glm::mat4(1.0f), Pos)
			* glm::rotate(glm::mat4(1.0f), glm::radians(RotationDeg), glm::vec3(0.0f, 0.0f, 1.0f))
			* glm::scale(glm::mat4(1.0f), glm::vec3(Size.x, Size.y, 1.0f));

		for (std::size_t Idx = 0; Idx < 4; Idx++) {
			QuadVertexBufferPtr->Position = Transform * QuadVertexPositions[Idx];
			QuadVertexBufferPtr->Color = Color;
			QuadVertexBufferPtr->TexCoord = TexCoords[Idx];
			QuadVertexBufferPtr->TexIndex = Texture.GetSlot();
			QuadVertexBufferPtr->TileFactor = TileFactor;
			QuadVertexBufferPtr->OutlineThickness = OutlineThickness;
			QuadVertexBufferPtr->OutlineColor = OutlineColor;
			QuadVertexBufferPtr++;
		}

		QuadIndexCount += 6;
		DrawStats.QuadCount++;
	}

	void CRenderer::DrawQuad(const glm::vec2& Pos, const glm::vec2& Size, const CTexture& Texture, const FSpriteUV& UV, const glm::vec4& Color, const float RotationDeg, float OutlineThickness, const glm::vec4& OutlineColor)
	{
		DrawQuad({Pos.x, Pos.y, 0.010f}, Size, Texture, UV, Color, RotationDeg, OutlineThickness, OutlineColor);
	}

	void CRenderer::DrawQuad(const glm::vec3& Pos, const glm::vec2& Size, const CTexture& Texture, const FSpriteUV& UV, const glm::vec4& Color, const float RotationDeg,
		const float OutlineThickness, const glm::vec4& OutlineColor)
	{
		if (QuadIndexCount >= MAX_INDICES) {
			NextBatch();
		}

		constexpr float TileFactor = 1.0f;

		const glm::mat4 Transform = glm::translate(glm::mat4(1.0f), Pos)
			* glm::rotate(glm::mat4(1.0f), glm::radians(RotationDeg), glm::vec3(0.0f, 0.0f, 1.0f))
			* glm::scale(glm::mat4(1.0f), {Size.x, Size.y, 1.0f});

		const std::array<glm::vec2, 4> TexCoords = {
			glm::vec2(UV.U0, UV.V0),
			glm::vec2(UV.U0, UV.V1),
			glm::vec2(UV.U1, UV.V1),
			glm::vec2(UV.U1, UV.V0)};

		for (std::size_t Idx = 0; Idx < 4; Idx++) {
			QuadVertexBufferPtr->Position = Transform * QuadVertexPositions[Idx];
			QuadVertexBufferPtr->Color = Color;
			QuadVertexBufferPtr->TexCoord = TexCoords[Idx];
			QuadVertexBufferPtr->TexIndex = Texture.GetSlot();
			QuadVertexBufferPtr->TileFactor = TileFactor;
			QuadVertexBufferPtr->OutlineThickness = OutlineThickness;
			QuadVertexBufferPtr->OutlineColor = OutlineColor;
			QuadVertexBufferPtr++;
		}

		QuadIndexCount += 6;
		DrawStats.QuadCount++;
	}

	void CRenderer::DrawQuad(const glm::vec2& Pos, const glm::vec2& Size, const ETexture Texture, const glm::vec4& Color, const float RotationDeg, const float OutlineThickness, const glm::vec4& OutlineColor)
	{
		DrawQuad(Pos, Size, *GetTexture(Texture), Color, RotationDeg, OutlineThickness, OutlineColor);
	}

	void CRenderer::DrawLine(const glm::vec2& P0, const glm::vec2& P1, const glm::vec4& Color, const uint16_t LineWidth)
	{
		DrawLine({P0.x, P0.y, 0.0f}, {P1.x, P1.y, 0.0f}, Color, LineWidth);
	}

	void CRenderer::DrawLine(const glm::vec3& P0, const glm::vec3& P1, const glm::vec4& Color, const uint16_t LineWidth)
	{
		if ((LineIndexCount + 2) > MAX_LINE_VERTICES) {
			NextBatch();
		}

		LineVertexBufferPtr->Position = P0;
		LineVertexBufferPtr->Color = Color;
		LineVertexBufferPtr++;

		LineVertexBufferPtr->Position = P1;
		LineVertexBufferPtr->Color = Color;
		LineVertexBufferPtr++;

		LineIndexCount += 2;
		DrawStats.LineCount++;
	}

	void CRenderer::DrawCircle(const glm::vec2& P0, const glm::vec3& Rotation, const float Radius, const glm::vec4& Color)
	{
		DrawCircle({P0.x, P0.y, 0.0f}, Rotation, Radius, Color);
	}

	void CRenderer::DrawCircle(const glm::vec3& P0, const glm::vec3& Rotation, const float Radius, const glm::vec4& Color)
	{
		const glm::mat4 Transform = glm::translate(glm::mat4(1.0f), P0)
			* glm::rotate(glm::mat4(1.0f), Rotation.x, {1.0f, 0.0f, 0.0f})
			* glm::rotate(glm::mat4(1.0f), Rotation.y, {0.0f, 1.0f, 0.0f})
			* glm::rotate(glm::mat4(1.0f), Rotation.z, {0.0f, 0.0f, 1.0f})
			* glm::scale(glm::mat4(1.0f), glm::vec3(Radius));

		DrawCircle(Transform, Color);
	}

	void CRenderer::DrawCircle(const glm::mat4& Transform, const glm::vec4& Color)
	{
		if ((LineIndexCount + (CIRCLE_SEGMENTS * 2)) > MAX_LINE_VERTICES) {
			NextBatch();
		}

		for (int Idx = 0; Idx < CIRCLE_SEGMENTS; Idx++) {
			float AngleRad = 2.0f * glm::pi<float>() * static_cast<float>(Idx) / CIRCLE_SEGMENTS;
			const glm::vec4 StartPos = {glm::cos(AngleRad), glm::sin(AngleRad), 0.0f, 1.0f};
			AngleRad = 2.0f * glm::pi<float>() * static_cast<float>((Idx + 1) % CIRCLE_SEGMENTS) / CIRCLE_SEGMENTS;
			const glm::vec4 EndPos = {glm::cos(AngleRad), glm::sin(AngleRad), 0.0f, 1.0f};

			const glm::vec3 P0 = Transform * StartPos;
			const glm::vec3 P1 = Transform * EndPos;
			DrawLine(P0, P1, Color);
		}
	}

	void CRenderer::DrawCircleFilled(const glm::vec2& P0, const float Radius, const glm::vec4& Color, const float Thickness)
	{
		DrawCircleFilled({P0.x, P0.y, 0.0f}, Radius, Color, Thickness);
	}

	void CRenderer::DrawCircleFilled(const glm::vec3& P0, const float Radius, const glm::vec4& Color, const float Thickness)
	{
		if ((CircleIndexCount + 6) > MAX_INDICES) {
			NextBatch();
		}

		const glm::mat4 Transform = glm::translate(glm::mat4(1.0f), P0)
			* glm::scale(glm::mat4(1.0f), glm::vec3(Radius * 2.0f, Radius * 2.0f, 1.0f));

		for (int Idx = 0; Idx < 4; Idx++) {
			CircleVertexBufferPtr->WorldPosition = Transform * QuadVertexPositions[Idx];
			CircleVertexBufferPtr->Thickness = Thickness;
			CircleVertexBufferPtr->LocalPosition = QuadVertexPositions[Idx] * 2.0f;
			CircleVertexBufferPtr->Color = Color;
			CircleVertexBufferPtr++;
		}

		CircleIndexCount += 6;
		DrawStats.QuadCount++;
	}

	void CRenderer::DrawTransform(const glm::mat4& Transform, const float Scale, const glm::vec4& Color)
	{
		glm::vec3 P0 = Transform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		glm::vec3 P1 = Transform * glm::vec4(Scale, 0.0f, 0.0f, 1.0f);
		DrawLine(P0, P1, Color);

		P1 = Transform * glm::vec4(0.0f, Scale, 0.0f, 1.0f);
		DrawLine(P0, P1, Color);

		P1 = Transform * glm::vec4(0.0f, 0.0f, Scale, 1.0f);
		DrawLine(P0, P1, Color);
	}

	const glm::vec4& CRenderer::GetClearColor()
	{
		return Data.ViewportFramebuffer->GetClearColor();
	}

	void CRenderer::SetClearColor(const glm::vec4& InClearColor)
	{
		Data.ViewportFramebuffer->SetClearColor(InClearColor);
	}

	void CRenderer::SetLineWidth(const uint16_t LineWidth)
	{
		LineConfig.Width = LineWidth;
		LK_OpenGL_Verify(glLineWidth(LineConfig.Width));
	}

	void CRenderer::SetDepthTest(const bool Enabled)
	{
		LK_TRACE_TAG("Renderer", "Depth test: {}", Enabled ? "Enabled" : "Disabled");
		Data.GL.bDepthTest = Enabled;
		if (Enabled) {
			LK_OpenGL_Verify(glEnable(GL_DEPTH_TEST));
		} else {
			LK_OpenGL_Verify(glDisable(GL_DEPTH_TEST));
		}
	}

	bool CRenderer::GetDepthTest()
	{
		return Data.GL.bDepthTest;
	}

	void CRenderer::SetDepthFunction(const uint32_t DepthFunc)
	{
		Data.GL.DepthFunc = DepthFunc;
		LK_OpenGL_Verify(glDepthFunc(Data.GL.DepthFunc));
	}

	uint32_t CRenderer::GetDepthFunction()
	{
		return Data.GL.DepthFunc;
	}

	const FDrawStatistics& CRenderer::GetDrawStatistics()
	{
		return DrawStats;
	}

	void CRenderer::ResetDrawStatistics()
	{
		std::memset(&DrawStats, 0, sizeof(DrawStats));
	}

	void CRenderer::SetCameraViewProjection(const glm::mat4& ViewProj)
	{
		CameraData.ViewProjection = ViewProj;
	}

	std::shared_ptr<CTexture> CRenderer::GetWhiteTexture()
	{
		return Data.WhiteTexture;
	}

	std::shared_ptr<CTexture> CRenderer::GetTexture(const ETexture Texture)
	{
		LK_ASSERT(Data.Textures.contains(Texture));
		return Data.Textures[Texture];
	}

	const std::unordered_map<ETexture, std::shared_ptr<CTexture>>& CRenderer::GetTextures()
	{
		return Data.Textures;
	}

	std::shared_ptr<CShader> CRenderer::GetShader(const CShader::EType ShaderType)
	{
		switch (ShaderType) {
			case CShader::EType::Quad:   return QuadShader;
			case CShader::EType::Line:   return LineShader;
			case CShader::EType::Circle: return CircleShader;
		}
		LK_VERIFY(false);
		return nullptr;
	}

	void CRenderer::SetBlending(const bool Enabled)
	{
		Data.GL.bBlending = Enabled;
		if (Enabled) {
			LK_OpenGL_Verify(glEnable(GL_BLEND));
		} else {
			LK_OpenGL_Verify(glDisable(GL_BLEND));
		}
	}

	void CRenderer::SetBlendFunction(const uint32_t Source, const uint32_t Destination)
	{
		Data.GL.BlendSource = Source;
		Data.GL.BlendDestination = Destination;
		LK_TRACE_TAG("Renderer", "Source={} Dst={}", Data.GL.BlendSource, Data.GL.BlendDestination);
		LK_OpenGL_Verify(glBlendFunc(Data.GL.BlendSource, Data.GL.BlendDestination));
	}

	uint32_t CRenderer::GetBlendSource()
	{
		return Data.GL.BlendSource;
	}

	uint32_t CRenderer::GetBlendDestination()
	{
		return Data.GL.BlendDestination;
	}

	std::pair<uint32_t, uint32_t> CRenderer::GetBlendFunction()
	{
		return std::make_pair(Data.GL.BlendSource, Data.GL.BlendDestination);
	}

	void CRenderer::SetDebugRender(const bool Enabled)
	{
		bDebugRender = Enabled;
	}

}
