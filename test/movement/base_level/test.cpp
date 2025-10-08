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
		constexpr float RectangleVertices[] = {
		  /*  Position    Texture Coordinates */
			-1.0f, -1.0f,    0.0f, 0.0f,
			 1.0f, -1.0f,    1.0f, 0.0f,
			 1.0f,  1.0f,    1.0f, 1.0f,
			-1.0f,  1.0f,    0.0f, 1.0f
		};

		constexpr uint32_t RectangleIndices[] = { 
			0, 1, 2,  
			2, 3, 0 
		};
	}

	CTest::CTest(const int Argc, char* Argv[])
		: CTestBase(Argc, Argv)
	{
	}

	void CTest::Run()
	{
		Running = true;
		const int CatchResult = Catch::Session().run(Args.Argc, Args.Argv);
		LK_DEBUG("Catch result: {}", CatchResult);

		const std::filesystem::path& BinaryDir = GetBinaryDirectory();
		const CWindow& Window = GetWindow();
		const FWindowData& WindowData = Window.GetData();


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

		const std::filesystem::path RectVertexShaderPath = BinaryDir / "vertex.shader";
		const std::filesystem::path RectFragmentShaderPath = BinaryDir / "frag.shader";
		CShader RectangleShader(RectVertexShaderPath, RectFragmentShaderPath);

		const char* WhiteTexturePath = TEXTURES_DIR "/white.png";
		const char* BricksTexturePath = TEXTURES_DIR "/bricks.jpg";
		LK_VERIFY(std::filesystem::exists(WhiteTexturePath));
		LK_VERIFY(std::filesystem::exists(BricksTexturePath));

		/*********************************
		 * Texture
		 *********************************/
		FTextureSpecification Spec = {
			.Path = WhiteTexturePath,
			.Width = 200,
			.Height = 200,
			.Format = EImageFormat::RGBA32F,
			.SamplerWrap = ETextureWrap::Clamp,
			.SamplerFilter = ETextureFilter::Nearest,
		};
		CTexture PlayerTexture(Spec);

		Spec.Path = BricksTexturePath;
		Spec.Width = 512;
		Spec.Height = 512;
		CTexture PlatformTexture(Spec);

		glm::vec4 ClearColor{ 0.10f, 0.10f, 0.10f, 1.0f };
		glm::vec4 FragColor{ 1.0f, 0.560f, 1.0f, 1.0f };

		auto HandleInput = [](glm::vec2& Position, const float MovementDiff) -> void
		{
			if (ImGui::IsKeyDown(ImGuiKey::ImGuiKey_A))
			{
				Position.x -= MovementDiff;
			}
			if (ImGui::IsKeyDown(ImGuiKey::ImGuiKey_D))
			{
				Position.x += MovementDiff;
			}
		};

		while (Running)
		{
			glfwPollEvents();
			glClearColor(ClearColor.r, ClearColor.g, ClearColor.b, ClearColor.a);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			CTest::ImGui_NewFrame();

			ImGui::Text("%s", LK_TEST_NAME);
			ImGui::Text("Resolution: %dx%d", WindowData.Width, WindowData.Height);
			ImGui::Text("Texture: %dx%d", PlayerTexture.GetWidth(), PlayerTexture.GetHeight());

			ImGui::SetNextItemWidth(320.0f);
			ImGui::SliderFloat3("Background", &ClearColor.x, 0.0f, 1.0f, "%.2f");

			const bool bTable = ImGui::BeginTable("##Table", 2);
			ImGui::TableSetupColumn("##Column1");
			ImGui::TableSetupColumn("##Column2");

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);

			/* -- Player -- */
			ImGui::SeparatorText("Player");
			ImGui::PushID(ImGui::GetID("Player"));
			ImGui::SliderFloat4("Color", &FragColor.x, 0.0f, 1.0f, "%.3f");
			static glm::vec2 PlayerPos{ -0.28f, -0.41f };
			ImGui::SliderFloat2("Position", &PlayerPos.x, -0.50f, 0.50f, "%.2f");
			static glm::vec2 PlayerScale{ 0.10f, 0.10f };
			ImGui::SliderFloat2("Scale", &PlayerScale.x, 0.01f, 0.30f, "%.2f");
			static float PlayerRot = 0.0f;
			ImGui::SliderFloat("Rotation", &PlayerRot, -180.0f, 180.0f, "%1.f", ImGuiSliderFlags_ClampOnInput);
			static float MovementSpeed = 0.00032f;
			ImGui::SliderFloat("Movement Speed", &MovementSpeed, 0.00015f, 0.00040f, "%.5f"); /* @todo: Convert to larger number */
			ImGui::PopID();

			HandleInput(PlayerPos, MovementSpeed);
			/* -- ~Player-- */

			ImGui::TableSetColumnIndex(1);

			/* Draw platform. */
			static glm::vec2 PlatformPos { -0.07f, -0.82f };
			static glm::vec4 PlatformFragColor{ 0.284f, 0.349f, 0.630f, 1.0f };
			ImGui::SeparatorText("Platform");
			ImGui::PushID(ImGui::GetID("Platform"));
			ImGui::SliderFloat4("Color", &PlatformFragColor.x, 0.0f, 1.0f, "%.3f");
			ImGui::SliderFloat2("Position", &PlatformPos.x, -2.0, 2.0f, "%.2f");
			static glm::vec2 PlatformScale{ 0.65f, 0.31f };
			ImGui::SliderFloat2("Scale", &PlatformScale.x, 0.01f, 2.0f, "%.2f");
			static float PlatformRot = 0.0f;
			ImGui::SliderFloat("Rotation", &PlatformRot, -180.0f, 180.0f, "%1.f", ImGuiSliderFlags_ClampOnInput);
			ImGui::PopID();

			glm::mat4 PlatformTransform = glm::mat4(1.0f);
			PlatformTransform = glm::translate(PlatformTransform, glm::vec3(PlatformPos, 0.0f))
				* glm::rotate(PlatformTransform, glm::radians(PlatformRot), glm::vec3(0.0f, 0.0f, 1.0f))
				* glm::scale(PlatformTransform, glm::vec3(PlatformScale, 1.0f));
			RectangleShader.Set("u_transform", PlatformTransform);
			PlatformTexture.Bind();
			RectangleShader.Set("u_texture", 0);
			RectangleShader.Set("u_color", PlatformFragColor);
			glBindVertexArray(RectangleVAO);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			PlatformTexture.Unbind();
			glBindVertexArray(0);

			/* Draw player. */
			glm::mat4 PlayerTransform = glm::mat4(1.0f);
			PlayerTransform = glm::translate(PlayerTransform, glm::vec3(PlayerPos, 0.0f))
				* glm::rotate(PlayerTransform, glm::radians(PlayerRot), glm::vec3(0.0f, 0.0f, 1.0f))
				* glm::scale(PlayerTransform, glm::vec3(PlayerScale, 1.0f));
			RectangleShader.Set("u_transform", PlayerTransform);

			PlayerTexture.Bind();
			RectangleShader.Set("u_texture", 0);
			RectangleShader.Set("u_color", FragColor);
			glBindVertexArray(RectangleVAO);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			PlayerTexture.Unbind();

			ImGui::EndTable();

			CTest::ImGui_EndFrame();
			glfwSwapBuffers(Window.GetGlfwWindow());
			glfwPollEvents();
		}
	}

	void CTest::Destroy()
	{
		glfwTerminate();
	}

}