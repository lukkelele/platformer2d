#include "test.h"

#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>
#include <stb/stb_image.h>

#include "core/assert.h"
#include "core/window.h"
#include "renderer/texture.h"
#include "renderer/shader.h"
#include "renderer/vertexbufferlayout.h"

namespace platformer2d::test {

	namespace 
	{
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

	CTest::CTest(const int Argc, char* Argv[])
		: CTestBase(Argc, Argv)
	{
		LK_ASSERT(Window && Window->GetGlfwWindow());
		InitRenderContext(Window->GetGlfwWindow());
		OpenGL::LoadInfo(BackendInfo);
		LK_INFO("OpenGL {}.{}", BackendInfo.Version.Major, BackendInfo.Version.Minor);
		LK_INFO("ImGui Version: {}", ImGui::GetVersion());
	}

	void CTest::Run()
	{
		bRunning = true;
		const int CatchResult = Catch::Session().run(Args.Argc, Args.Argv);
		LK_DEBUG("Catch result: {}", CatchResult);

		const std::filesystem::path& BinaryDir = GetBinaryDirectory();
		const CWindow& Window = GetWindow();
		const FWindowData& WindowData = Window.GetData();


		/*********************************
		 * Triangle
		 *********************************/
		GLuint TriangleVAO = OpenGL::VertexArray::Create();

		/**
		 * Buffer layout.
		 * Must match the data passed to the vertex buffer.
		 */
		const FVertexBufferLayout TriangleLayout = {
			{ "pos", EShaderDataType::Float3, },
		};

		static_assert(sizeof(TriangleVertices) > 0);
		GLuint TriangleVBO = OpenGL::VertexBuffer::Create(TriangleVertices, TriangleLayout);
		glBindVertexArray(0);

		const std::filesystem::path VertexShaderPath = BinaryDir / "vertex.shader";
		const std::filesystem::path FragmentShaderPath = BinaryDir / "frag.shader";
		CShader TriangleShader(VertexShaderPath, FragmentShaderPath);


		/*********************************
		 * Rectangle
		 *********************************/
		const FVertexBufferLayout RectangleLayout = {
			{ "pos",      EShaderDataType::Float2, },
			{ "texcoord", EShaderDataType::Float2, },
		};

		GLuint RectangleVAO = OpenGL::VertexArray::Create();
		GLuint RectangleVBO = OpenGL::VertexBuffer::Create(RectangleVertices, RectangleLayout);
		/* Create element array buffer for the rectangle indices. */
		GLuint RectangleEBO = OpenGL::ElementBuffer::Create(RectangleIndices);

		glBindVertexArray(0);

		const std::filesystem::path RectVertexShaderPath = BinaryDir / "rect_vertex.shader";
		const std::filesystem::path RectFragmentShaderPath = BinaryDir / "rect_frag.shader";
		CShader RectangleShader(RectVertexShaderPath, RectFragmentShaderPath);

		const char* TexturePath = TEXTURES_DIR "/bricks.jpg";
		LK_VERIFY(std::filesystem::exists(TexturePath));


		/*********************************
		 * Texture
		 *********************************/
		stbi_uc* TextureData = nullptr;
		int ReadWidth, ReadHeight, ReadChannels;
		TextureData = (uint8_t*)stbi_loadf(TexturePath, &ReadWidth, &ReadHeight, &ReadChannels, 4);
		LK_INFO("Texture: {}x{}", ReadWidth, ReadHeight);
		LK_ASSERT(TextureData && (ReadWidth > 0) && (ReadHeight > 0), "Corrupt texture");
		CTexture Texture(ReadWidth, ReadHeight, TextureData);
		const uint32_t TextureID = Texture.GetRendererID();

		glm::vec4 ClearColor{ 0.10f, 0.10f, 0.10f, 1.0f };
		glm::vec4 FragColor{ 0.410f, 0.181f, 0.813f, 1.0f };

		while (bRunning)
		{
			glfwPollEvents();
			glClearColor(ClearColor.r, ClearColor.g, ClearColor.b, ClearColor.a);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			CTest::ImGui_NewFrame();

			ImGui::Text("%s", LK_TEST_NAME);
			ImGui::Text("Resolution: %dx%d", WindowData.Width, WindowData.Height);
			ImGui::Text("Texture: %dx%d", ReadWidth, ReadHeight);
			ImGui::Text("Channels: %d", ReadChannels);

			ImGui::SetNextItemWidth(320.0f);
			ImGui::SliderFloat3("Background", &ClearColor.x, 0.0f, 1.0f, "%.2f");

			const bool bTable = ImGui::BeginTable("##Table", 2);
			ImGui::TableSetupColumn("##Column1");
			ImGui::TableSetupColumn("##Column2");

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);

			/* -- Triangle -- */
			ImGui::SeparatorText("Triangle");
			ImGui::PushID(ImGui::GetID("Triangle"));
			const bool UpdateFragShader = ImGui::SliderFloat4("Fragment Shader", &FragColor.x, 0.0f, 1.0f, "%.3f");

			/* Calculate triangle transform. */
			static glm::vec2 TrianglePos{ 0.0f };
			ImGui::SliderFloat2("Position", &TrianglePos.x, -0.50f, 0.50f, "%.2f");
			static glm::vec2 TriangleScale{ 1.0f, 1.0f };
			ImGui::SliderFloat2("Scale", &TriangleScale.x, 0.10f, 2.50f, "%.1f");
			glm::mat4 TriangleTransform = glm::mat4(1.0f);
			static float TriangleRot = 0.0f;
			ImGui::SliderFloat("Rotation", &TriangleRot, 0.0f, 360.0f, "%1.f", ImGuiSliderFlags_ClampOnInput);
			TriangleTransform = glm::translate(TriangleTransform, glm::vec3(TrianglePos, 0.0f));
			TriangleTransform = glm::scale(TriangleTransform, glm::vec3(TriangleScale, 1.0f));
			TriangleTransform = glm::rotate(TriangleTransform, glm::radians(TriangleRot), glm::vec3(0.0f, 0.0f, 1.0f));
			TriangleShader.Set("u_transform", TriangleTransform);

			ImGui::PopID();
			/* -- ~Triangle -- */

			ImGui::TableSetColumnIndex(1);

			/* -- Rectangle -- */
			ImGui::SeparatorText("Rectangle");
			ImGui::PushID(ImGui::GetID("Rectangle"));

			/* Calculate rectangle transform. */
			static glm::vec2 RectPos{ -0.28f, -0.41f };
			ImGui::SliderFloat2("Position", &RectPos.x, -0.50f, 0.50f, "%.2f");
			static glm::vec2 RectScale{ 1.0f, 1.0f };
			ImGui::SliderFloat2("Scale", &RectScale.x, 0.10f, 2.50f, "%.1f");
			glm::mat4 RectTransform = glm::mat4(1.0f);
			static float RectRot = 0.0f;
			ImGui::SliderFloat("Rotation", &RectRot, 0.0f, 360.0f, "%1.f", ImGuiSliderFlags_ClampOnInput);
			RectTransform = glm::translate(RectTransform, glm::vec3(RectPos, 0.0f));
			RectTransform = glm::scale(RectTransform, glm::vec3(RectScale, 1.0f));
			RectTransform = glm::rotate(RectTransform, glm::radians(RectRot), glm::vec3(0.0f, 0.0f, 1.0f));
			RectangleShader.Set("u_transform", RectTransform);

			ImGui::PopID();
			/* -- ~Rectangle -- */

			/* Draw rectangle. */
			Texture.Bind();
			RectangleShader.Set("u_texture", 0);
			glBindVertexArray(RectangleVAO);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			Texture.Unbind();

			/* Draw triangle. */
			TriangleShader.Set("u_color", FragColor);
			glBindVertexArray(TriangleVAO);
			glDrawArrays(GL_TRIANGLES, 0, 3);

			ImGui::EndTable();

			CTest::ImGui_EndFrame();
			glfwSwapBuffers(Window.GetGlfwWindow());
		}
	}

	void CTest::Destroy()
	{
		glfwTerminate();
	}

}