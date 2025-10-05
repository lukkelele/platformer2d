#include "test.h"

#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <stb/stb_image.h>

#include "core/assert.h"
#include "core/window.h"
#include "renderer/texture.h"
#include "renderer/shader.h"
#include "renderer/vertexbufferlayout.h"

namespace platformer2d::test {

	namespace {
		constexpr float TriangleVertices[] = {
			 0.0f,   0.25f,  0.0f,
			 0.25f, -0.25f,  0.0f,
			-0.25f, -0.25f,  0.0f,
		};

		constexpr float RectangleVertices[] = {
#ifdef RECTANGLE_UNSCALED
		  /* Position    Texture Coordinates */
			-1.0f, -1.0f,    0.0f, 0.0f,
			 1.0f, -1.0f,    1.0f, 0.0f,
			 1.0f,  1.0f,    1.0f, 1.0f,
			-1.0f,  1.0f,    0.0f, 1.0f
#else
		  /* Position    Texture Coordinates */
			-0.30f, -0.30f,   0.0f, 0.0f,
			 0.30f, -0.30f,   1.0f, 0.0f,
			 0.30f,  0.30f,   1.0f, 1.0f,
			-0.30f,  0.30f,   0.0f, 1.0f
#endif
		};

		constexpr uint32_t RectangleIndices[] = { 
			0, 1, 2,  
			2, 3, 0 
		};
	}

	static void AddVertexBufferToVertexArray(const GLuint VAO, const GLuint VBO, const FVertexBufferLayout& Layout);

	CTest::CTest(const int Argc, char* Argv[])
		: CTestBase(Argc, Argv)
	{
	}

	void CTest::Run()
	{
		Running = true;
		const int Result = Catch::Session().run(Args.Argc, Args.Argv);
		const std::filesystem::path& BinaryDir = GetBinaryDirectory();
		const CWindow& Window = GetWindow();
		const FWindowData& WindowData = Window.GetData();

		/* Create vertex array. */
		GLuint TriangleVAO;
		glGenVertexArrays(1, &TriangleVAO);
		glBindVertexArray(TriangleVAO);

		/* Add vertex buffer. */
		static_assert(sizeof(TriangleVertices) > 0);
		GLuint TriangleVBO;
		glGenBuffers(1, &TriangleVBO);
		glBindBuffer(GL_ARRAY_BUFFER, TriangleVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(TriangleVertices), TriangleVertices, GL_STATIC_DRAW);

		/**
		 * Buffer layout.
		 * Must match the data passed to the vertex buffer.
		 */
		const FVertexBufferLayout TriangleLayout = {
			{ "pos", EShaderDataType::Float3, },
		};

		/* Configure the vertex array based on the buffer layout. */
		AddVertexBufferToVertexArray(TriangleVAO, TriangleVBO, TriangleLayout);
		glBindVertexArray(0);

		const std::filesystem::path VertexShaderPath = BinaryDir / "vertex.shader";
		const std::filesystem::path FragmentShaderPath = BinaryDir / "frag.shader";
		CShader TriangleShader(VertexShaderPath, FragmentShaderPath);

		/* Create rectangle vertex array and buffer. */
		const FVertexBufferLayout RectangleLayout = {
			{ "pos",      EShaderDataType::Float2, },
			{ "texcoord", EShaderDataType::Float4, },
		};

		GLuint RectangleVAO;
		glGenVertexArrays(1, &RectangleVAO);
		glBindVertexArray(RectangleVAO);

		GLuint RectangleVBO;
		glGenBuffers(1, &RectangleVBO);
		glBindBuffer(GL_ARRAY_BUFFER, RectangleVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(RectangleVertices), RectangleVertices, GL_STATIC_DRAW);

		/* Create element array buffer for rectangle. */
		GLuint RectangleEBO;
		glGenBuffers(1, &RectangleEBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, RectangleEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(RectangleIndices), RectangleIndices, GL_STATIC_DRAW);

		/* Position. */
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
		glEnableVertexAttribArray(0);
		/* Texture coordinates. */
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
		glEnableVertexAttribArray(1);
		glBindVertexArray(0);

		const std::filesystem::path RectVertexShaderPath = BinaryDir / "rect_vertex.shader";
		const std::filesystem::path RectFragmentShaderPath = BinaryDir / "rect_frag.shader";
		CShader RectangleShader(RectVertexShaderPath, RectFragmentShaderPath);

		const char* TexturePath = ASSETS_DIR "/textures/bricks.jpg";
		LK_VERIFY(std::filesystem::exists(TexturePath));

		stbi_uc* TextureData = nullptr;
		int ReadWidth, ReadHeight, ReadChannels;
		TextureData = (uint8_t*)stbi_loadf(TexturePath, &ReadWidth, &ReadHeight, &ReadChannels, 4);
		LK_INFO("Texture: {}x{}", ReadWidth, ReadHeight);
		LK_ASSERT(TextureData && (ReadWidth > 0) && (ReadHeight > 0), "Corrupt texture");
		CTexture Texture(ReadWidth, ReadHeight, TextureData);
		const uint32_t TextureID = Texture.GetRendererID();

		glm::vec4 ClearColor{ 0.10f, 0.10f, 0.10f, 1.0f };
		glm::vec4 FragColor{ 0.410f, 0.181f, 0.813f, 1.0f };

		while (Running)
		{
			glfwPollEvents();
			glClearColor(ClearColor.r, ClearColor.g, ClearColor.b, ClearColor.a);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			CTest::ImGui_NewFrame();

			ImGui::Text("%s", LK_TEST_NAME);
			ImGui::Text("Resolution: %dx%d", WindowData.Width, WindowData.Height);
			ImGui::Text("Texture: %dx%d", ReadWidth, ReadHeight);
			ImGui::Text("Channels: %d", ReadChannels);
			ImGui::Dummy(ImVec2(0, 5));

			/* Background */
			ImGui::SliderFloat3("Background", &ClearColor.x, 0.0f, 1.0f, "%.2f");

			/* -- Triangle -- */
			ImGui::Dummy(ImVec2(0, 20));
			ImGui::SeparatorText("Triangle");
			ImGui::PushID(ImGui::GetID("Triangle"));
			const bool UpdateFragShader = ImGui::SliderFloat4("Fragment Shader", &FragColor.x, 0.0f, 1.0f, "%.3f");
			static glm::vec2 Offset{ 0.0f, 0.0f };
			ImGui::SliderFloat2("Position Offset", &Offset.x, -0.50f, 0.50f, "%.2f");
			ImGui::PopID();
			/* -- ~Triangle -- */

			/* -- Rectangle -- */
			ImGui::Dummy(ImVec2(0, 20));
			ImGui::SeparatorText("Rectangle");
			ImGui::PushID(ImGui::GetID("Rectangle"));
			static glm::vec2 RectOffset = { -0.12f, -0.24f };
			ImGui::SliderFloat2("Position Offset", &RectOffset.x, -0.50f, 0.50f, "%.2f");
			ImGui::PopID();
			/* -- ~Rectangle -- */

			/* Draw rectangle. */
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, TextureID);
			RectangleShader.Set("u_offset", RectOffset);
			RectangleShader.Set("u_texture", 0);
			glBindVertexArray(RectangleVAO);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			glBindTexture(GL_TEXTURE_2D, 0);

			/* Draw triangle. */
			TriangleShader.Set("u_offset", Offset);
			TriangleShader.Set("u_color", FragColor);
			glBindVertexArray(TriangleVAO);
			glDrawArrays(GL_TRIANGLES, 0, 3);

			CTest::ImGui_EndFrame();
			glfwSwapBuffers(Window.GetGlfwWindow());
			glfwPollEvents();
		}
	}

	void CTest::Destroy()
	{
		glfwTerminate();
	}

	static void AddVertexBufferToVertexArray(const GLuint VAO, const GLuint VBO, const FVertexBufferLayout& Layout)
	{
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		int VertexBufferIndex = 0;
		for (const FVertexBufferElement& Element : Layout)
		{
			LK_INFO("VertexBufferIndex={} Size={} ComponentCount={}", VertexBufferIndex, Element.Size, Element.GetComponentCount());
			switch (Element.Type)
			{
				case EShaderDataType::Float:
				case EShaderDataType::Float2:
				case EShaderDataType::Float3:
				case EShaderDataType::Float4:
				{
					glEnableVertexAttribArray(VertexBufferIndex);
					glVertexAttribPointer(VertexBufferIndex,
										  Element.GetComponentCount(),
										  OpenGL::ShaderDataTypeToOpenGLBaseType(Element.Type),
										  (Element.Normalized ? GL_TRUE : GL_FALSE),
										  Layout.GetStride(),
										  (const void*)Element.Offset);
					VertexBufferIndex++;
					break;
				}
				case EShaderDataType::Int:
				case EShaderDataType::Int2:
				case EShaderDataType::Int3:
				case EShaderDataType::Int4:
				case EShaderDataType::Bool:
				{
					glEnableVertexAttribArray(VertexBufferIndex);
					glVertexAttribIPointer(VertexBufferIndex,
										   Element.GetComponentCount(),
										   OpenGL::ShaderDataTypeToOpenGLBaseType(Element.Type),
										   Layout.GetStride(),
										   (const void*)Element.Offset);
					VertexBufferIndex++;
					break;
				}
				case EShaderDataType::Mat3:
				case EShaderDataType::Mat4:
				{
					uint8_t Count = Element.GetComponentCount();
					for (uint8_t Idx = 0; Idx < Count; Idx++)
					{
						glEnableVertexAttribArray(VertexBufferIndex);
						glVertexAttribPointer(VertexBufferIndex,
											  Count, 
											  OpenGL::ShaderDataTypeToOpenGLBaseType(Element.Type), 
											  (Element.Normalized ? GL_TRUE : GL_FALSE),
											  Layout.GetStride(),
											  (const void*)(Element.Offset + sizeof(float) * Count * Idx));
						glVertexAttribDivisor(VertexBufferIndex, 1);
						VertexBufferIndex++;
					}
					break;
				}

				default: 
					LK_ERROR("Unhandled shader type: {}", Enum::ToString(Element.Type));
			}
		}
	}

}