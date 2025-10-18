#include "renderer.h"

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
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <spdlog/spdlog.h>

#include "core/window.h"
#include "backendinfo.h"
#include "imguilayer.h"
#include "opengl.h"
#include "rendercommandqueue.h"

namespace platformer2d {

	constexpr int MAX_TEXTURES = 16;

	struct FRendererData
	{
		std::shared_ptr<CTexture> WhiteTexture = nullptr;
		std::array<std::shared_ptr<CTexture>, MAX_TEXTURES> Textures = { nullptr };
	};

	namespace 
	{
		constexpr uint32_t MaxQuads = 10000;
		constexpr uint32_t MaxLines = 1000;
		constexpr uint32_t MaxVertices = MaxQuads * 4;
		constexpr uint32_t MaxIndices = MaxQuads * 6;
		constexpr uint32_t MaxLineVertices = MaxLines * 2;
		constexpr uint32_t MaxLineIndices = MaxLines * 6;

		FRendererData Data{};

		std::array<CRenderCommandQueue*, 2> CommandQueue;
		std::atomic<uint32_t> CommandQueueSubmissionIndex = 0;

		constexpr glm::vec2 QuadTextureCoords[] = {
			{ 0.0f, 0.0f }, /*  Bottom Left.  */
			{ 0.0f, 1.0f }, /*  Top Left.     */
			{ 1.0f, 1.0f }, /*  Top Right.    */
			{ 1.0f, 0.0f }  /*  Bottom Right. */
		};

		FDrawStatistics DrawStats;
	}

	void CRenderer::Initialize()
	{
		const GLenum GladInitResult = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		LK_OpenGL_Verify(glEnable(GL_BLEND));
		LK_OpenGL_Verify(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
		LK_OpenGL_Verify(glEnable(GL_DEPTH_TEST));
		LK_OpenGL_Verify(glDepthFunc(GL_ALWAYS)); /* @todo Change to GL_LESS */
		LK_OpenGL_Verify(glEnable(GL_LINE_SMOOTH));

		OpenGL::LoadInfo(BackendInfo);
		LK_INFO("OpenGL {}.{}", BackendInfo.Version.Major, BackendInfo.Version.Minor);
#ifdef LK_LOG_OPENGL_EXTENSIONS
		for (const std::string& Extension : BackendInfo.Extensions)
		{
			LK_INFO("Extension: {}", Extension);
		}
#endif

#ifdef LK_BUILD_DEBUG
		OpenGL::Internal::SetupDebugContext(nullptr);
#endif

		for (int Idx = 0; Idx < CommandQueue.size(); Idx++)
		{
			CommandQueue[Idx] = new CRenderCommandQueue();
		}

		/* Quad */
		{
			QuadVertexPositions[0] = { -0.50f, -0.50f, 0.0f, 1.0f };
			QuadVertexPositions[1] = { -0.50f,  0.50f, 0.0f, 1.0f };
			QuadVertexPositions[2] = {  0.50f,  0.50f, 0.0f, 1.0f };
			QuadVertexPositions[3] = {  0.50f, -0.50f, 0.0f, 1.0f };

			const FVertexBufferLayout QuadLayout = {
				{ "pos",        EShaderDataType::Float3, },
				{ "color",      EShaderDataType::Float4, },
				{ "texcoord",   EShaderDataType::Float2, },
				{ "texindex",   EShaderDataType::Int,  },
				{ "tilefactor", EShaderDataType::Float,  },
			};

			QuadVAO = OpenGL::VertexArray::Create();
			QuadVBO = OpenGL::VertexBuffer::Create(MaxVertices * sizeof(FQuadVertex), QuadLayout);

			QuadVertexBufferBase = new FQuadVertex[MaxVertices];
			uint32_t* QuadIndices = new uint32_t[MaxIndices];
			LK_VERIFY(QuadIndices, "Failed to alloc QuadIndices on the heap");
			uint32_t Offset = 0;
			for (uint32_t Idx = 0; Idx < MaxIndices; Idx += 6)
			{
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

			QuadEBO = OpenGL::ElementBuffer::Create(QuadIndices, MaxIndices * sizeof(uint32_t));
			delete[] QuadIndices;

			QuadVertexBufferPtr = QuadVertexBufferBase;
			LK_VERIFY(QuadVertexBufferPtr);

			QuadShader = std::make_shared<CShader>(SHADERS_DIR "/quad.shader");

			std::array<int, MAX_TEXTURES> Slots = { 0 };
			for (int Idx = 0; Idx < Slots.size(); Idx++)
			{
				Slots[Idx] = Idx;
			}
			QuadShader->Set("u_textures", Slots);

			CameraData.ViewProjection = glm::mat4(1.0f);
			CameraUniformBuffer = std::make_unique<CUniformBuffer>(sizeof(FCameraData));
			CameraUniformBuffer->SetBinding(QuadShader, "ub_camera", 0);
			CameraUniformBuffer->SetData(&CameraData, sizeof(FCameraData));
		}

		/* Line */
		{
			const FVertexBufferLayout LineLayout = {
				{ "pos",   EShaderDataType::Float3, },
				{ "color", EShaderDataType::Float4, },
			};

			LineVAO = OpenGL::VertexArray::Create();
			LineVBO = OpenGL::VertexBuffer::Create(MaxVertices * sizeof(FQuadVertex), LineLayout);
			LineVertexBufferBase = new FLineVertex[MaxVertices];

			uint32_t* LineIndices = new uint32_t[MaxLineIndices];
			for (uint32_t Idx = 0; Idx < MaxLineIndices; Idx++)
			{
				LineIndices[Idx] = Idx;
			}
			LineEBO = OpenGL::ElementBuffer::Create(LineIndices, MaxLineIndices * sizeof(uint32_t));
			delete[] LineIndices;

			LineVertexBufferPtr = LineVertexBufferBase;
			LK_VERIFY(LineVertexBufferPtr);

			LineShader = std::make_shared<CShader>(SHADERS_DIR "/line.shader");

			LK_OpenGL_Verify(glLineWidth(LineConfig.Width));
		}

		/* Textures */
		{
			const char* WhiteTexturePath = TEXTURES_DIR "/white.png";
			LK_VERIFY(std::filesystem::exists(WhiteTexturePath));
			LK_TRACE_TAG("Renderer", "Load white texture");
			FTextureSpecification WhiteTextureSpec = {
				.Path = WhiteTexturePath,
				.Width = 2048,
				.Height = 2048,
				.Format = EImageFormat::RGBA32F,
				.SamplerWrap = ETextureWrap::Clamp,
				.SamplerFilter = ETextureFilter::Nearest,
			};
			Data.WhiteTexture = std::make_shared<CTexture>(WhiteTextureSpec);
			Data.Textures[0] = Data.WhiteTexture;

			const char* PlayerTexturePath = TEXTURES_DIR "/test/test_player.png";
			LK_VERIFY(std::filesystem::exists(PlayerTexturePath));
			FTextureSpecification PlayerTextureSpec = {
				.Path = PlayerTexturePath,
				.Width = 200,
				.Height = 200,
				.bFlipVertical = true,
				.Format = EImageFormat::RGBA32F,
				.SamplerWrap = ETextureWrap::Clamp,
				.SamplerFilter = ETextureFilter::Nearest,
			};
			Data.Textures[1] = std::make_shared<CTexture>(PlayerTextureSpec);
		}

		for (int Idx = 0; Idx < Data.Textures.size(); Idx++)
		{
			if (Data.Textures[Idx])
			{
				Data.Textures[Idx]->Bind(Idx);
			}
		}

		ImGuiLayer = std::make_unique<CImGuiLayer>(CWindow::Get()->GetGlfwWindow());
	}

	void CRenderer::Destroy()
	{
		Data.WhiteTexture = nullptr;
		std::shared_ptr<CTexture> Texture = nullptr;
		for (int Idx = 0; Idx < Data.Textures.size(); Idx++)
		{
			if (Texture = Data.Textures[Idx]; Texture != nullptr)
			{
				Texture->Unbind(Idx);
				Texture.reset();
			}
		}
	}

	void CRenderer::BeginFrame()
	{
		LK_OpenGL_Verify(glClearColor(ClearColor.r, ClearColor.g, ClearColor.b, ClearColor.a));
		LK_OpenGL_Verify(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

		ImGuiLayer->BeginFrame();
		StartBatch();
	}

	void CRenderer::EndFrame()
	{
		ImGuiLayer->EndFrame();
	}

	void CRenderer::BeginScene(const CCamera& Camera)
	{
		CameraData.ViewProjection = Camera.GetViewProjection();
		CameraUniformBuffer->SetData(&CameraData, sizeof(FCameraData));

		StartBatch();
	}

	void CRenderer::BeginScene(const CCamera& Camera, const glm::mat4& Transform)
	{
		CameraData.ViewProjection = Camera.GetViewProjection() * glm::inverse(Transform);
		CameraUniformBuffer->SetData(&CameraData, sizeof(FCameraData));

		StartBatch();
	}

	void CRenderer::EndScene()
	{
		Flush();
	}

	void CRenderer::StartBatch()
	{
		QuadIndexCount = 0;
		QuadVertexBufferPtr = QuadVertexBufferBase;

		LineIndexCount = 0;
		LineVertexBufferPtr = LineVertexBufferBase;
	}

	void CRenderer::NextBatch()
	{
		Flush();
		StartBatch();
	}

	void CRenderer::Flush()
	{
		if (QuadIndexCount > 0)
		{
			/* Compute byte count. */
			const uint32_t DataSize = static_cast<uint32_t>((uint8_t*)QuadVertexBufferPtr - (uint8_t*)QuadVertexBufferBase);
			LK_OpenGL_Verify(glBindBuffer(GL_ARRAY_BUFFER, QuadVBO));
			LK_OpenGL_Verify(glBufferSubData(GL_ARRAY_BUFFER, 0, DataSize, QuadVertexBufferBase));

			QuadShader->Bind();
			Data.WhiteTexture->Bind(0);
			LK_OpenGL_Verify(glBindVertexArray(QuadVAO));
			LK_OpenGL_Verify(glDrawElements(GL_TRIANGLES, QuadIndexCount, GL_UNSIGNED_INT, nullptr));
			Data.WhiteTexture->Unbind(0);
			QuadShader->Unbind();
		}

		if (LineIndexCount > 0)
		{
			/* Compute byte count. */
			const uint32_t DataSize = static_cast<uint32_t>((uint8_t*)LineVertexBufferPtr - (uint8_t*)LineVertexBufferBase);
			LK_OpenGL_Verify(glBindBuffer(GL_ARRAY_BUFFER, LineVBO));
			LK_OpenGL_Verify(glBufferSubData(GL_ARRAY_BUFFER, 0, DataSize, LineVertexBufferBase));

			LineShader->Bind();
			LineShader->Set("u_proj", glm::mat4(1.0f));
			Data.WhiteTexture->Bind(0);
			LK_OpenGL_Verify(glBindVertexArray(LineVAO));
			LK_OpenGL_Verify(glDrawElements(GL_LINES, LineIndexCount, GL_UNSIGNED_INT, nullptr));
			Data.WhiteTexture->Unbind(0);
			LineShader->Unbind();
		}
	}

	void CRenderer::SubmitQuad(const glm::vec2& Pos, const glm::vec2& Size,
							   const glm::vec4& Color, const float RotationDeg)
	{
		if (QuadIndexCount >= MaxIndices)
		{
			NextBatch();
		}

		static constexpr int TextureIndex = 0;
		static constexpr float TileFactor = 1.0f;

		const glm::mat4 Transform = glm::translate(glm::mat4(1.0f), { Pos.x, Pos.y, 0.0f })
            * glm::scale(glm::mat4(1.0f), { Size.x, Size.y, 1.0f });

		for (std::size_t i = 0; i < 4; i++)
		{
			QuadVertexBufferPtr->Position = Transform * QuadVertexPositions[i];
			QuadVertexBufferPtr->Color = Color;
			QuadVertexBufferPtr->TexCoord = QuadTextureCoords[i];
			QuadVertexBufferPtr->TexIndex = TextureIndex;
			QuadVertexBufferPtr->TileFactor = TileFactor;
			QuadVertexBufferPtr++;
		}

		QuadIndexCount += 6;
	}

	void CRenderer::SubmitQuad(const glm::vec2& Pos, const glm::vec2& Size, CTexture& Texture,
							   const glm::vec4& Color, const float RotationDeg)
	{
		if (QuadIndexCount >= MaxIndices)
		{
			NextBatch();
		}

		static constexpr float TileFactor = 1.0f;

		const glm::mat4 Transform = glm::translate(glm::mat4(1.0f), { Pos.x, Pos.y, 0.0f })
            * glm::scale(glm::mat4(1.0f), { Size.x, Size.y, 1.0f });

		for (std::size_t i = 0; i < 4; i++)
		{
			QuadVertexBufferPtr->Position = Transform * QuadVertexPositions[i];
			QuadVertexBufferPtr->Color = Color;
			QuadVertexBufferPtr->TexCoord = QuadTextureCoords[i];
			QuadVertexBufferPtr->TexIndex = Texture.GetIndex();
			QuadVertexBufferPtr->TileFactor = TileFactor;
			QuadVertexBufferPtr++;
		}

		QuadIndexCount += 6;
		DrawStats.QuadCount++;
	}

	void CRenderer::SubmitLine(const glm::vec2& P0, const glm::vec2& P1, const uint16_t LineWidth, const glm::vec4& Color)
	{
		SubmitLine({ P0.x, P0.y, 0.0f }, { P1.x, P1.y, 0.0f }, LineWidth, Color);
	}

	void CRenderer::SubmitLine(const glm::vec3& P0, const glm::vec3& P1, const uint16_t LineWidth, const glm::vec4& Color)
	{
		static constexpr glm::mat4 Proj = glm::mat4(1.0f);

		LineVertexBufferPtr->Position = P0;
		LineVertexBufferPtr->Color = Color;
		LineVertexBufferPtr++;

		LineVertexBufferPtr->Position = P1;
		LineVertexBufferPtr->Color = Color;
		LineVertexBufferPtr++;

		LineIndexCount += 2;
		DrawStats.LineCount++;
	}

	void CRenderer::DrawLine(const glm::vec2& P0, const glm::vec2& P1, const uint16_t LineWidth, const glm::vec4& Color)
	{
		DrawLine({ P0.x, P0.y, 0.0f }, { P1.x, P1.y, 0.0f }, LineWidth, Color);
	}

	void CRenderer::DrawLine(const glm::vec3& P0, const glm::vec3& P1, const uint16_t LineWidth, const glm::vec4& Color)
	{
		LineShader->Set("u_transform", glm::mat4(1.0f));
		LineShader->Set("u_color", Color);

		const float Vertices[2][2] = { { P0.x, P0.y }, { P1.x, P1.y } };
		LK_OpenGL_Verify(glBindBuffer(GL_ARRAY_BUFFER, LineVBO));
		LK_OpenGL_Verify(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertices), Vertices));

		LK_OpenGL_Verify(glBindVertexArray(LineVAO));
		LK_OpenGL_Verify(glLineWidth(LineWidth));
		LK_OpenGL_Verify(glDrawArrays(GL_LINES, 0, 2));
	}

	void CRenderer::SetLineWidth(const uint16_t LineWidth)
	{
		LineConfig.Width = LineWidth;
		LK_OpenGL_Verify(glLineWidth(LineConfig.Width));
	}

	const FDrawStatistics& CRenderer::GetDrawStatistics()
	{
		return DrawStats;
	}

	void CRenderer::ResetDrawStatistics()
	{
		std::memset(&DrawStats, 0, sizeof(DrawStats));
	}

}