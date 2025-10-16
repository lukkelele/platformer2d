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

	struct FRendererData
	{
		struct FLine
		{
			GLuint VAO = 0;
			GLuint VBO = 0;
		} Line;
		std::unique_ptr<CShader> LineShader = nullptr;

		std::shared_ptr<CTexture> WhiteTexture = nullptr;
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
	}

	void CRenderer::Initialize()
	{
		const GLenum GladInitResult = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		LK_OpenGL_Verify(glEnable(GL_BLEND));
		LK_OpenGL_Verify(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
		LK_OpenGL_Verify(glEnable(GL_DEPTH_TEST));
		LK_OpenGL_Verify(glEnable(GL_LINE_SMOOTH));

		OpenGL::LoadInfo(BackendInfo);
		LK_INFO("OpenGL {}.{}", BackendInfo.Version.Major, BackendInfo.Version.Minor);
#ifdef LK_LOG_OPENGL_EXTENSIONS
		for (const std::string& Extension : BackendInfo.Extensions)
		{
			LK_INFO("Extension: {}", Extension);
		}
#endif

		LK_DEBUG_TAG("Renderer", "Creating {} render command queues", CommandQueue.size());
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
				{ "pos",        EShaderDataType::Float4, },
				{ "color",      EShaderDataType::Float4, },
				{ "texcoord",   EShaderDataType::Float2, },
				{ "texindex",   EShaderDataType::Float,  },
				{ "tilefactor", EShaderDataType::Float,  },
			};

			/* Quad VAO. */
			LK_OpenGL_Verify(glGenVertexArrays(1, &QuadVAO));
			LK_OpenGL_Verify(glBindVertexArray(QuadVAO));

			/* Quad VBO. */
			LK_OpenGL_Verify(glGenBuffers(1, &QuadVBO));
			LK_OpenGL_Verify(glBindBuffer(GL_ARRAY_BUFFER, QuadVBO));
			LK_OpenGL_Verify(glBufferData(GL_ARRAY_BUFFER, MaxVertices * sizeof(FQuadVertex), nullptr, GL_STATIC_DRAW));

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(
				0,
				3,
				GL_FLOAT,
				GL_FALSE,
				sizeof(FQuadVertex),
				(const void*)(offsetof(FQuadVertex, Position)) // (const void*)0
			);
			static_assert(0 * sizeof(float) == offsetof(FQuadVertex, Position));

			glEnableVertexAttribArray(1);
			glVertexAttribPointer(
				1,
				4,
				GL_FLOAT,
				GL_FALSE,
				sizeof(FQuadVertex),
				(const void*)(offsetof(FQuadVertex, Color)) // (const void*)(3 * sizeof(float))
			);
			static_assert(3 * sizeof(float) == offsetof(FQuadVertex, Color));

			glEnableVertexAttribArray(2);
			glVertexAttribPointer(
				2,
				2,
				GL_FLOAT,
				GL_FALSE,
				sizeof(FQuadVertex),
				(const void*)(offsetof(FQuadVertex, TexCoord))
			);
			static_assert(7 * sizeof(float) == offsetof(FQuadVertex, TexCoord));

			glEnableVertexAttribArray(3);
			glVertexAttribPointer(
				3,
				1,
				GL_FLOAT,
				GL_FALSE,
				sizeof(FQuadVertex),
				(const void*)(offsetof(FQuadVertex, TexIndex)) // (const void*)(9 * sizeof(float))
			);
			static_assert(9 * sizeof(float) == offsetof(FQuadVertex, TexIndex));

			glEnableVertexAttribArray(4);
			glVertexAttribPointer(
				4,
				1,
				GL_FLOAT,
				GL_FALSE,
				sizeof(FQuadVertex),
				(const void*)(offsetof(FQuadVertex, TileFactor)) // (const void*)(10 * sizeof(float))
			);
			static_assert(10 * sizeof(float) == offsetof(FQuadVertex, TileFactor));

			QuadShader = std::make_unique<CShader>(SHADERS_DIR "/quad.shader");
			QuadShader->Set("u_proj", glm::mat4(1.0f));
			QuadShader->Set("u_texarray", 0); /* @fixme */

			QuadVertexBufferBase = new FQuadVertex[MaxVertices];
			uint32_t* QuadIndices = new uint32_t[MaxIndices];
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

			/* Quad EBO. */
			LK_OpenGL_Verify(glCreateBuffers(1, &QuadEBO));
			LK_OpenGL_Verify(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, QuadEBO));
			LK_OpenGL_Verify(glNamedBufferData(QuadEBO, MaxIndices * sizeof(uint32_t), QuadIndices, GL_STATIC_DRAW));

			QuadVertexBufferPtr = QuadVertexBufferBase;

			delete[] QuadIndices;
		}

		/* Line */
		{
			glGenVertexArrays(1, &Data.Line.VAO);
			glGenBuffers(1, &Data.Line.VBO);

			glBindVertexArray(Data.Line.VAO);
			glBindBuffer(GL_ARRAY_BUFFER, Data.Line.VBO);
			/* Two vectors: 2 * 2 * sizeof(float) */
			glBufferData(GL_ARRAY_BUFFER, (2 * 2 * sizeof(float)), nullptr, GL_DYNAMIC_DRAW);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, (2 * sizeof(float)), nullptr);
			glEnableVertexAttribArray(0);

			Data.LineShader = std::make_unique<CShader>(SHADERS_DIR "/line.shader");
		}

		const char* WhiteTexturePath = TEXTURES_DIR "/white.png";
		LK_TRACE_TAG("Renderer", "Load white texture");
		FTextureSpecification WhiteTextureSpec = {
			.Path = WhiteTexturePath,
			.Width = 200,
			.Height = 200,
			.Format = EImageFormat::RGBA32F,
			.SamplerWrap = ETextureWrap::Clamp,
			.SamplerFilter = ETextureFilter::Nearest,
		};
		Data.WhiteTexture = std::make_shared<CTexture>(WhiteTextureSpec);

		ImGuiLayer = std::make_unique<CImGuiLayer>(CWindow::Get()->GetGlfwWindow());
	}

	void CRenderer::BeginFrame()
	{
		LK_OpenGL_Verify(glClearColor(ClearColor.r, ClearColor.g, ClearColor.b, ClearColor.a));
		LK_OpenGL_Verify(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
	
		ImGuiLayer->BeginFrame();
	}

	void CRenderer::EndFrame()
	{
		ImGuiLayer->EndFrame();
	}

	void CRenderer::StartBatch()
	{
		QuadIndexCount = 0;
		QuadVertexBufferPtr = QuadVertexBufferBase;
	}

	void CRenderer::NextBatch()
	{
		Flush();
		StartBatch();
	}

	void CRenderer::Flush()
	{
		if (QuadIndexCount)
		{
			const uint32_t DataSize = static_cast<uint32_t>((uint8_t*)QuadVertexBufferPtr - (uint8_t*)QuadVertexBufferBase);
			LK_OpenGL_Verify(glBindBuffer(GL_ARRAY_BUFFER, QuadVBO));
			LK_OpenGL_Verify(glBufferSubData(GL_ARRAY_BUFFER, 0, DataSize, QuadVertexBufferBase));

			QuadShader->Bind();
			Data.WhiteTexture->Bind();
			LK_OpenGL_Verify(glBindVertexArray(QuadVAO));
			LK_OpenGL_Verify(glDrawElements(GL_TRIANGLES, QuadIndexCount, GL_UNSIGNED_INT, nullptr));
			Data.WhiteTexture->Unbind();
			QuadShader->Unbind();
		}
	}

	void CRenderer::DrawQuad(const glm::vec2& Pos, const glm::vec2& Size, 
							 const glm::vec4& Color, const float RotationDeg)
	{
		if (QuadIndexCount >= MaxIndices)
		{
			NextBatch();
		}

		static constexpr float TextureIndex = 0.0f;
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

	void CRenderer::DrawLine(const glm::vec2& P1, const glm::vec2& P2,
							 const uint16_t LineWidth, const glm::vec4& Color)
	{
		Data.LineShader->Set("u_transform", glm::mat4(1.0f));
		Data.LineShader->Set("u_color", { 1.0f, 0.50f, 1.0f, 1.0f });

		const float Vertices[2][2] = { { P1.x, P1.y }, { P2.x, P2.y } };
		LK_OpenGL_Verify(glBindBuffer(GL_ARRAY_BUFFER, Data.Line.VBO));
		LK_OpenGL_Verify(glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertices), Vertices));

		LK_OpenGL_Verify(glBindVertexArray(Data.Line.VAO));
		LK_OpenGL_Verify(glLineWidth(LineWidth));
		LK_OpenGL_Verify(glDrawArrays(GL_LINES, 0, 2));
	}

}