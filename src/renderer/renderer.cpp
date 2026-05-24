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

#include "core/profiler.h"
#include "core/settings.h"
#include "core/string.h"
#include "core/timer.h"
#include "core/window.h"
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
	static constexpr std::uint32_t MAX_TEXT_GLYPHS = 4096;
	static constexpr std::uint32_t MAX_TEXT_VERTICES = MAX_TEXT_GLYPHS * 4;
	static constexpr std::uint32_t MAX_TEXT_INDICES = MAX_TEXT_GLYPHS * 6;

	CRenderer::FLineConfig CRenderer::LineConfig;
	CRenderer::FCameraData CRenderer::CameraData;

	namespace {
		struct FRendererData
		{
			std::uint16_t FrameIndex = 0;
			std::uint16_t RefreshRate = 0;
			std::shared_ptr<CTexture> WhiteTexture = nullptr;
			std::shared_ptr<CFramebuffer> ViewportFramebuffer = nullptr;
			std::map<ETexture, std::shared_ptr<CTexture>> Textures;
			std::map<ETexture, FSpriteSheet> SpriteSheets;

			struct
			{
				bool bBlending = false;
				std::uint32_t BlendSource = 0;
				std::uint32_t BlendDestination = 0;
				bool bDepthTest = false;
				std::uint32_t DepthFunc = 0;
			} GL;
		};
	}

	static FRendererData Data{};
	static FDrawStatistics DrawStats;

	static std::array<CRenderCommandQueue*, 2> CommandQueue;
	static std::atomic_uint8_t CommandQueueSubmissionIndex = 0;
	static const std::size_t CommandQueueCount = CommandQueue.size();

	static constexpr glm::vec2 QuadTextureCoords[] = {
		{0.0f, 0.0f}, /*  Bottom Left.  */
		{0.0f, 1.0f}, /*  Top Left.     */
		{1.0f, 1.0f}, /*  Top Right.    */
		{1.0f, 0.0f}  /*  Bottom Right. */
	};

	static void EmitTextGlyphs(const CFontAtlas& Font, std::string_view Text, const glm::vec3& Origin, float Scale,
		const glm::vec4& Color, const glm::vec4& OutlineColor, float OutlineWidth, FTextVertex*& VertexPtr,
		std::uint32_t& IndexCount, std::uint32_t MaxIndices, float YSign, std::uint64_t& OutGlyphCount);

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
		LoadFonts();

		CreateFramebuffer();
		SetupQuadRenderer();
		SetupLineRenderer();
		SetupCircleRenderer();
		SetupTextRenderer();

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

		delete[] TextWorldVertexBufferBase;
		TextWorldVertexBufferBase = nullptr;
		TextWorldVertexBufferPtr = nullptr;
		delete[] TextScreenVertexBufferBase;
		TextScreenVertexBufferBase = nullptr;
		TextScreenVertexBufferPtr = nullptr;
		DefaultFont.reset();
		for (auto& Row : Fonts) {
			for (std::shared_ptr<CFontAtlas>& FontRef : Row) {
				FontRef.reset();
			}
		}
		TextShader.reset();
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

	void CRenderer::SetupTextRenderer()
	{
		const FVertexBufferLayout TextLayout = {
			/* clang-format off */
			{ "pos",          EShaderDataType::Float3, },
			{ "color",        EShaderDataType::Float4, },
			{ "texcoord",     EShaderDataType::Float2, },
			{ "outlinecolor", EShaderDataType::Float4, },
			{ "outlinewidth", EShaderDataType::Float,  },
			/* clang-format on */
		};

		TextWorldVAO = OpenGL::VertexArray::Create();
		TextWorldVBO = OpenGL::VertexBuffer::Create(MAX_TEXT_VERTICES * sizeof(FTextVertex), TextLayout);
		TextWorldVertexBufferBase = new FTextVertex[MAX_TEXT_VERTICES];
		TextWorldVertexBufferPtr = TextWorldVertexBufferBase;

		std::uint32_t* TextIndices = new std::uint32_t[MAX_TEXT_INDICES];
		std::uint32_t Offset = 0;
		for (std::uint32_t Idx = 0; Idx < MAX_TEXT_INDICES; Idx += 6) {
			TextIndices[Idx + 0] = Offset + 0;
			TextIndices[Idx + 1] = Offset + 1;
			TextIndices[Idx + 2] = Offset + 2;
			TextIndices[Idx + 3] = Offset + 2;
			TextIndices[Idx + 4] = Offset + 3;
			TextIndices[Idx + 5] = Offset + 0;
			Offset += 4;
		}
		const GLuint TextEBO = OpenGL::ElementBuffer::Create(TextIndices, MAX_TEXT_INDICES * sizeof(std::uint32_t));

		TextScreenVAO = OpenGL::VertexArray::Create();
		TextScreenVBO = OpenGL::VertexBuffer::Create(MAX_TEXT_VERTICES * sizeof(FTextVertex), TextLayout);
		TextScreenVertexBufferBase = new FTextVertex[MAX_TEXT_VERTICES];
		TextScreenVertexBufferPtr = TextScreenVertexBufferBase;

		/* Both text VAOs reuse the same index buffer. */
		LK_OpenGL_Verify(glBindVertexArray(TextWorldVAO));
		LK_OpenGL_Verify(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, TextEBO));
		LK_OpenGL_Verify(glBindVertexArray(TextScreenVAO));
		LK_OpenGL_Verify(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, TextEBO));
		LK_OpenGL_Verify(glBindVertexArray(0));

		delete[] TextIndices;

		TextShader = std::make_shared<CShader>(SHADERS_DIR "/text.shader");
		TextShader->Bind();
		TextShader->Set("u_atlas", FontAtlasTextureSlot);
		TextShader->Unbind();
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
		auto LoadTexture = [](std::string_view Path, const ETexture Texture,
							   const EImageFormat Format = EImageFormat::RGBA8,
							   const glm::vec2& Size = {0.0f, 0.0f}) -> void
		{
			LK_VERIFY(std::filesystem::exists(Path), "Texture {} has no path", Enum::ToString(Texture));
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

		auto LoadSpriteSheet = [&](const ETexture Texture) -> void
		{
			LK_VERIFY(!Data.SpriteSheets.contains(Texture), "Sprite sheet already loaded: {}", Enum::ToString(Texture));
			const std::string SpritePath = std::format("{}/sprites/{}.lsprite", TEXTURES_DIR, Enum::ToString(Texture));
			if (!std::filesystem::exists(SpritePath)) {
				LK_TRACE_TAG("Renderer", "No sprite sheet for: {}", Enum::ToString(Texture));
				return;
			}

			FSpriteReader Reader;
			std::optional<FSpriteSheet> LoadedSheet = Reader.Read(SpritePath);
			LK_VERIFY(LoadedSheet, "Failed to load: {} ({})", Enum::ToString(Texture), StringUtils::GetPathRelativeToProject(SpritePath));
			LK_DEBUG_TAG("Renderer", "Load sprite: {} ({})", Enum::ToString(Texture), StringUtils::GetPathRelativeToProject(SpritePath));
			Data.SpriteSheets.emplace(Texture, std::move(*LoadedSheet));
		};

		LoadTexture(TEXTURES_DIR "/white.png", ETexture::White, EImageFormat::RGBA8, {1.0f, 1.0f});
		LoadTexture(TEXTURES_DIR "/characters.png", ETexture::Player, EImageFormat::RGBA8);
		LoadTexture(TEXTURES_DIR "/metal.png", ETexture::Metal, EImageFormat::RGBA8);
		LoadTexture(TEXTURES_DIR "/bricks.png", ETexture::Bricks, EImageFormat::RGBA8);
		LoadTexture(TEXTURES_DIR "/wood.png", ETexture::Wood, EImageFormat::RGBA8);
		LoadTexture(TEXTURES_DIR "/swoosh.png", ETexture::Swoosh, EImageFormat::RGBA8);
		LoadTexture(TEXTURES_DIR "/cloud-1.png", ETexture::Cloud, EImageFormat::RGBA8);
		LoadTexture(TEXTURES_DIR "/ar15.png", ETexture::Rifle, EImageFormat::RGBA8);
		LoadTexture(TEXTURES_DIR "/white.png", ETexture::Axe, EImageFormat::RGBA8);
		LoadTexture(TEXTURES_DIR "/goblin.png", ETexture::Goblin, EImageFormat::RGBA8);
		Data.WhiteTexture = Data.Textures[ETexture::White];
		LK_INFO_TAG("Renderer", "Loaded {} textures", Data.Textures.size());

		for (const auto& [Texture, TextureRef] : Data.Textures) {
			LoadSpriteSheet(Texture);
		}
		LK_INFO_TAG("Renderer", "Loaded {} sprite sheets", Data.SpriteSheets.size());
	}

	void CRenderer::LoadFonts()
	{
		const std::filesystem::path FontsRoot = FONTS_DIR;
		if (!std::filesystem::exists(FontsRoot)) {
			LK_WARN_TAG("Renderer", "Fonts directory missing: {}", FontsRoot);
			return;
		}

		for (const auto& Entry : std::filesystem::recursive_directory_iterator(FontsRoot)) {
			if (!Entry.is_regular_file()) {
				continue;
			}
			const std::filesystem::path& JsonPath = Entry.path();
			const std::string FileName = JsonPath.filename().string();
			if (!FileName.ends_with(".msdf.json")) {
				continue;
			}

			const std::string BaseName = FileName.substr(0, FileName.size() - std::string_view(".msdf.json").size());
			const std::filesystem::path PngPath = JsonPath.parent_path() / (BaseName + ".msdf.png");
			if (!std::filesystem::exists(PngPath)) {
				LK_WARN_TAG("Renderer", "Font JSON without matching PNG: {}", JsonPath);
				continue;
			}
			const std::size_t DashPos = BaseName.find('-');
			std::string_view Family;
			std::string_view Variant;
			if (DashPos == std::string::npos) {
				Family = BaseName;
				Variant = std::string_view{};
			} else {
				Family = std::string_view(BaseName).substr(0, DashPos);
				Variant = std::string_view(BaseName).substr(DashPos + 1);
			}

			const EFont Font = Enum::FromString<EFont>(Family);
			const EFontModifier Modifier = (Variant.empty() || (Variant == "Regular"))
				? EFontModifier::Normal
				: Enum::FromString<EFontModifier>(Variant);
			if ((Font == EFont::None) || (Modifier == EFontModifier::COUNT)) {
				LK_WARN_TAG("Renderer", "Skipping font with unknown family/modifier: {} ({}, {})", BaseName, Family, Variant);
				continue;
			}

			const std::size_t FontIdx = static_cast<std::size_t>(Font);
			const std::size_t ModIdx = static_cast<std::size_t>(Modifier);
			LK_ASSERT(FontIdx < Fonts.size(), "Font index out of range (FontIdx={})", FontIdx);
			LK_ASSERT(ModIdx < Fonts[FontIdx].size(), "Font modifier index out of range (ModIdx={})", ModIdx);
			Fonts[FontIdx][ModIdx] = std::make_shared<CFontAtlas>(JsonPath, PngPath);
		}

		const std::size_t RobotoIdx = static_cast<std::size_t>(EFont::Roboto);
		const std::size_t NormalIdx = static_cast<std::size_t>(EFontModifier::Normal);
		DefaultFont = Fonts[RobotoIdx][NormalIdx];
		if (!DefaultFont) {
			LK_WARN_TAG("Renderer", "Default font not found (needs bake)");
		}
	}

	const CFontAtlas& CRenderer::GetFont(const EFont Font, const EFontModifier Modifier)
	{
		const std::size_t FontIdx = static_cast<std::size_t>(Font);
		const std::size_t ModIdx = static_cast<std::size_t>(Modifier);
		LK_ASSERT((FontIdx < Fonts.size()) && (ModIdx < Fonts[FontIdx].size()) && Fonts.at(FontIdx).at(ModIdx));
		return *Fonts.at(FontIdx).at(ModIdx);
	}

	const CFontAtlas& CRenderer::GetDefaultFont()
	{
		return *DefaultFont;
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

		TextWorldIndexCount = 0;
		TextWorldVertexBufferPtr = TextWorldVertexBufferBase;
		TextWorldFont = nullptr;

		TextScreenIndexCount = 0;
		TextScreenVertexBufferPtr = TextScreenVertexBufferBase;
		TextScreenFont = nullptr;
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

		if ((TextWorldIndexCount > 0) && TextWorldFont && TextShader) {
			const std::uint32_t DataSize = static_cast<std::uint32_t>((std::uint8_t*)TextWorldVertexBufferPtr - (std::uint8_t*)TextWorldVertexBufferBase);
			LK_OpenGL_Verify(glBindBuffer(GL_ARRAY_BUFFER, TextWorldVBO));
			LK_OpenGL_Verify(glBufferSubData(GL_ARRAY_BUFFER, 0, DataSize, TextWorldVertexBufferBase));

			TextShader->Bind();
			TextShader->Set("u_viewproj", CameraData.ViewProjection);
			TextShader->Set("u_brightness", Brightness);
			TextShader->Set("u_pxrange", TextWorldFont->GetDistanceRange());
			TextShader->Set("u_atlas", FontAtlasTextureSlot);

			LK_OpenGL_Verify(glActiveTexture(GL_TEXTURE0 + FontAtlasTextureSlot));
			LK_OpenGL_Verify(glBindTexture(GL_TEXTURE_2D, TextWorldFont->GetTexture()->GetID()));

			LK_OpenGL_Verify(glBindVertexArray(TextWorldVAO));
			LK_OpenGL_Verify(glDrawElements(GL_TRIANGLES, TextWorldIndexCount, GL_UNSIGNED_INT, nullptr));
			TextShader->Unbind();
		}

		if ((TextScreenIndexCount > 0) && TextScreenFont && TextShader) {
			const std::uint32_t DataSize = static_cast<std::uint32_t>((std::uint8_t*)TextScreenVertexBufferPtr - (std::uint8_t*)TextScreenVertexBufferBase);
			LK_OpenGL_Verify(glBindBuffer(GL_ARRAY_BUFFER, TextScreenVBO));
			LK_OpenGL_Verify(glBufferSubData(GL_ARRAY_BUFFER, 0, DataSize, TextScreenVertexBufferBase));

			const float Width = static_cast<float>(Data.ViewportFramebuffer->GetWidth());
			const float Height = static_cast<float>(Data.ViewportFramebuffer->GetHeight());
			const glm::mat4 ScreenVP = glm::ortho(0.0f, Width, Height, 0.0f, -1.0f, 1.0f);

			const bool DepthWas = Data.GL.bDepthTest;
			if (DepthWas) {
				SetDepthTest(false);
			}

			TextShader->Bind();
			TextShader->Set("u_viewproj", ScreenVP);
			TextShader->Set("u_brightness", Brightness);
			TextShader->Set("u_pxrange", TextScreenFont->GetDistanceRange());
			TextShader->Set("u_atlas", FontAtlasTextureSlot);

			LK_OpenGL_Verify(glActiveTexture(GL_TEXTURE0 + FontAtlasTextureSlot));
			LK_OpenGL_Verify(glBindTexture(GL_TEXTURE_2D, TextScreenFont->GetTexture()->GetID()));

			LK_OpenGL_Verify(glBindVertexArray(TextScreenVAO));
			LK_OpenGL_Verify(glDrawElements(GL_TRIANGLES, TextScreenIndexCount, GL_UNSIGNED_INT, nullptr));
			TextShader->Unbind();

			if (DepthWas) {
				SetDepthTest(true);
			}
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

	void CRenderer::DrawCrossMark(const glm::vec2& Pos, const glm::vec4& Color, const std::uint16_t LineWidth, const float CrossArm, const float MarkerRadius)
	{
		DrawCrossMark(glm::vec3(Pos, 0.0f), Color, LineWidth, CrossArm, MarkerRadius);
	}

	void CRenderer::DrawCrossMark(const glm::vec3& Pos, const glm::vec4& Color, const std::uint16_t LineWidth, const float CrossArm, const float MarkerRadius)
	{
		const glm::vec3 SpV3 = {Pos.x, Pos.y, 0.0f};
		const glm::vec3 LeftV = {Pos.x - CrossArm, Pos.y, 0.0f};
		const glm::vec3 RightV = {Pos.x + CrossArm, Pos.y, 0.0f};
		const glm::vec3 BotV = {Pos.x, Pos.y - CrossArm, 0.0f};
		const glm::vec3 TopV = {Pos.x, Pos.y + CrossArm, 0.0f};
		DrawLine(LeftV, RightV, Color, LineWidth);
		DrawLine(BotV, TopV, Color, LineWidth);
		DrawCircleFilled(SpV3, MarkerRadius, Color);
	}

	void CRenderer::DrawText(const CFontAtlas& Font, const std::string_view Text, const glm::vec3& Pos,
		const float Scale, const glm::vec4& Color, const glm::vec4& OutlineColor, const float OutlineWidth)
	{
		LK_ASSERT(!Text.empty(), "Pos={} Scale={}", Pos, Scale);
		if (TextWorldFont && (TextWorldFont != &Font)) {
			NextBatch();
		}

		TextWorldFont = &Font;
		EmitTextGlyphs(Font, Text, Pos, Scale, Color, OutlineColor, OutlineWidth,
			TextWorldVertexBufferPtr, TextWorldIndexCount, MAX_TEXT_INDICES, 1.0f,
			DrawStats.GlyphCount);
	}

	void CRenderer::DrawText(const std::string_view Text, const glm::vec3& Pos, const float Scale,
		const glm::vec4& Color, const glm::vec4& OutlineColor, const float OutlineWidth)
	{
		LK_ASSERT(DefaultFont);
		DrawText(*DefaultFont, Text, Pos, Scale, Color, OutlineColor, OutlineWidth);
	}

	void CRenderer::DrawText(EFont Font, const std::string_view Text, const glm::vec2& Pos, const float Scale,
		const glm::vec4& Color, const glm::vec4& OutlineColor, const float OutlineWidth)
	{
		DrawText(Font, EFontModifier::Normal, Text, glm::vec3(Pos, 0.0f), Scale, Color, OutlineColor, OutlineWidth);
	}

	void CRenderer::DrawText(EFont Font, const std::string_view Text, const glm::vec3& Pos, const float Scale,
		const glm::vec4& Color, const glm::vec4& OutlineColor, const float OutlineWidth)
	{
		DrawText(Font, EFontModifier::Normal, Text, Pos, Scale, Color, OutlineColor, OutlineWidth);
	}

	void CRenderer::DrawText(EFont Font, EFontModifier FontMod, const std::string_view Text, const glm::vec2& Pos, const float Scale,
		const glm::vec4& Color, const glm::vec4& OutlineColor, const float OutlineWidth)
	{
		DrawText(Font, FontMod, Text, glm::vec3(Pos, 0.0f), Scale, Color, OutlineColor, OutlineWidth);
	}

	void CRenderer::DrawText(EFont Font, EFontModifier FontMod, const std::string_view Text, const glm::vec3& Pos, const float Scale,
		const glm::vec4& Color, const glm::vec4& OutlineColor, const float OutlineWidth)
	{
		LK_ASSERT(!Text.empty(), "Pos={} Scale={}", Pos, Scale);
		const CFontAtlas& FontRef = GetFont(Font, FontMod);
		if (TextWorldFont && (TextWorldFont != &FontRef)) {
			NextBatch();
		}

		TextWorldFont = &FontRef;
		EmitTextGlyphs(FontRef, Text, Pos, Scale, Color, OutlineColor, OutlineWidth,
			TextWorldVertexBufferPtr, TextWorldIndexCount, MAX_TEXT_INDICES, 1.0f,
			DrawStats.GlyphCount);
	}

	void CRenderer::DrawTextScreen(const CFontAtlas& Font, const std::string_view Text, const glm::vec2& PixelPos,
		const float PixelSize, const glm::vec4& Color, const glm::vec4& OutlineColor, const float OutlineWidth)
	{
		LK_ASSERT(!Text.empty(), "PixelPos={} PixelSize={}", PixelPos, PixelSize);
		if (TextScreenFont && (TextScreenFont != &Font)) {
			NextBatch();
		}

		TextScreenFont = &Font;
		const glm::vec3 Origin = {PixelPos.x, PixelPos.y, 0.0f};
		EmitTextGlyphs(Font, Text, Origin, PixelSize, Color, OutlineColor, OutlineWidth,
			TextScreenVertexBufferPtr, TextScreenIndexCount, MAX_TEXT_INDICES, -1.0f,
			DrawStats.GlyphCount);
	}

	void CRenderer::DrawTextScreen(const std::string_view Text, const glm::vec2& PixelPos, const float PixelSize,
		const glm::vec4& Color, const glm::vec4& OutlineColor, const float OutlineWidth)
	{
		LK_ASSERT(DefaultFont);
		DrawTextScreen(*DefaultFont, Text, PixelPos, PixelSize, Color, OutlineColor, OutlineWidth);
	}

	void CRenderer::DrawTextScreen(const EFont Font, const std::string_view Text, const glm::vec2& PixelPos,
		const float PixelSize, const glm::vec4& Color, const glm::vec4& OutlineColor, const float OutlineWidth)
	{
		DrawTextScreen(Font, EFontModifier::Normal, Text, PixelPos, PixelSize, Color, OutlineColor, OutlineWidth);
	}

	void CRenderer::DrawTextScreen(const EFont Font, const EFontModifier FontMod, const std::string_view Text, const glm::vec2& PixelPos,
		const float PixelSize, const glm::vec4& Color, const glm::vec4& OutlineColor, const float OutlineWidth)
	{
		LK_ASSERT(!Text.empty(), "PixelPos={} PixelSize={}", PixelPos, PixelSize);
		const CFontAtlas& FontRef = GetFont(Font, FontMod);
		if (TextScreenFont && (TextScreenFont != &FontRef)) {
			NextBatch();
		}

		TextScreenFont = &FontRef;
		const glm::vec3 Origin = {PixelPos.x, PixelPos.y, 0.0f};
		EmitTextGlyphs(FontRef, Text, Origin, PixelSize, Color, OutlineColor, OutlineWidth,
			TextScreenVertexBufferPtr, TextScreenIndexCount, MAX_TEXT_INDICES, -1.0f,
			DrawStats.GlyphCount);
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
		LK_ASSERT(Data.Textures.contains(Texture), "Not loaded: {}", Enum::ToString(Texture));
		return Data.Textures[Texture];
	}

	ETexture CRenderer::GetTexture(const std::filesystem::path& Path)
	{
		for (const auto& [Texture, Ref] : Data.Textures) {
			if (Ref) {
				const std::filesystem::path TexturePath = StringUtils::GetPathRelativeToAssetsDir(Ref->GetFilePath());
				if (Path == TexturePath) {
					return Texture;
				}
			}
		}
		return ETexture::White;
	}

	const std::map<ETexture, std::shared_ptr<CTexture>>& CRenderer::GetTextures()
	{
		return Data.Textures;
	}

	const FSpriteSheet* CRenderer::GetSpriteSheet(const ETexture Texture)
	{
		const auto Iter = Data.SpriteSheets.find(Texture);
		return (Iter != Data.SpriteSheets.end()) ? &Data.SpriteSheets.find(Texture)->second : nullptr;
	}

	FSpriteSheet* CRenderer::GetSpriteSheetMutable(const ETexture Texture)
	{
		const auto Iter = Data.SpriteSheets.find(Texture);
		return (Iter != Data.SpriteSheets.end()) ? &Iter->second : nullptr;
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

	static void EmitTextGlyphs(const CFontAtlas& Font, const std::string_view Text, const glm::vec3& Origin, const float Scale,
		const glm::vec4& Color, const glm::vec4& OutlineColor, const float OutlineWidth, FTextVertex*& VertexPtr, std::uint32_t& IndexCount,
		const std::uint32_t MaxIndices, const float YSign, std::uint64_t& OutGlyphCount)
	{
		const FFontMetrics& Metrics = Font.GetMetrics();
		glm::vec2 Cursor = {Origin.x, Origin.y};

		for (std::size_t Idx = 0; Idx < Text.size(); Idx++) {
			const char Ch = Text[Idx];
			if (Ch == '\n') {
				Cursor.x = Origin.x;
				Cursor.y -= Metrics.LineHeight * Scale * YSign;
				continue;
			}

			const std::uint32_t Codepoint = static_cast<std::uint8_t>(Ch);
			const FGlyph& Glyph = Font.GetGlyph(Codepoint);

			if (Glyph.bVisible) {
				if ((IndexCount + 6) > MaxIndices) {
					break;
				}

				const float X0 = Cursor.x + Glyph.PlaneMin.x * Scale;
				const float X1 = Cursor.x + Glyph.PlaneMax.x * Scale;
				const float Y0 = Cursor.y + Glyph.PlaneMin.y * Scale * YSign;
				const float Y1 = Cursor.y + Glyph.PlaneMax.y * Scale * YSign;
				const float Z = Origin.z;

				VertexPtr->Position = {X0, Y0, Z};
				VertexPtr->TexCoord = {Glyph.AtlasMin.x, Glyph.AtlasMin.y};
				VertexPtr->Color = Color;
				VertexPtr->OutlineColor = OutlineColor;
				VertexPtr->OutlineWidth = OutlineWidth;
				VertexPtr++;

				VertexPtr->Position = {X0, Y1, Z};
				VertexPtr->TexCoord = {Glyph.AtlasMin.x, Glyph.AtlasMax.y};
				VertexPtr->Color = Color;
				VertexPtr->OutlineColor = OutlineColor;
				VertexPtr->OutlineWidth = OutlineWidth;
				VertexPtr++;

				VertexPtr->Position = {X1, Y1, Z};
				VertexPtr->TexCoord = {Glyph.AtlasMax.x, Glyph.AtlasMax.y};
				VertexPtr->Color = Color;
				VertexPtr->OutlineColor = OutlineColor;
				VertexPtr->OutlineWidth = OutlineWidth;
				VertexPtr++;

				VertexPtr->Position = {X1, Y0, Z};
				VertexPtr->TexCoord = {Glyph.AtlasMax.x, Glyph.AtlasMin.y};
				VertexPtr->Color = Color;
				VertexPtr->OutlineColor = OutlineColor;
				VertexPtr->OutlineWidth = OutlineWidth;
				VertexPtr++;

				IndexCount += 6;
				OutGlyphCount++;
			}

			Cursor.x += Glyph.Advance * Scale;
		}
	}

}
