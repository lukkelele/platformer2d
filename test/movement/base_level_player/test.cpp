#include "test.h"

#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_common.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/norm.hpp>
#include <spdlog/spdlog.h>
#include <stb/stb_image.h>

#include "core/assert.h"
#include "core/window.h"
#include "renderer/renderer.h"
#include "renderer/shader.h"
#include "renderer/texture.h"
#include "renderer/vertexbufferlayout.h"

#include "game/player.h"
#include "core/input/keyboard.h"

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

	void UI_BlendFunction();

	CTest::CTest(const int Argc, char* Argv[])
		: CTestBase(Argc, Argv)
	{
		CRenderer::Initialize();
		CKeyboard::Initialize();
	}

	void CTest::Run()
	{
		Running = true;
		const int CatchResult = Catch::Session().run(Args.Argc, Args.Argv);
		LK_DEBUG("Catch result: {}", CatchResult);

		const std::filesystem::path& BinaryDir = GetBinaryDirectory();
		CWindow& Window = GetWindow();
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

		/*********************************
		 * Shader
		 *********************************/
		CShader Shader(BinaryDir / "player.shader");

		/*********************************
		 * Texture
		 *********************************/
		const char* WhiteTexturePath = TEXTURES_DIR "/white.png";
		const char* TestPlayerTexturePath = TEXTURES_DIR "/test/test_player.png";
		const char* BricksTexturePath = TEXTURES_DIR "/bricks.jpg";
		LK_VERIFY(std::filesystem::exists(WhiteTexturePath));
		LK_VERIFY(std::filesystem::exists(TestPlayerTexturePath));
		LK_VERIFY(std::filesystem::exists(BricksTexturePath));

		FTextureSpecification Spec = {
			.Path = TestPlayerTexturePath,
			.Width = 200,
			.Height = 200,
			.bFlipVertical = true,
			.Format = EImageFormat::RGBA32F,
			.SamplerWrap = ETextureWrap::Clamp,
			.SamplerFilter = ETextureFilter::Nearest,
		};
		CTexture PlayerTexture(Spec);

		Spec.Path = BricksTexturePath;
		Spec.Width = 512;
		Spec.Height = 512;
		CTexture PlatformTexture(Spec);

		/*********************************
		 * Player
		 *********************************/
		FActorSpecification PlayerSpec;
		/* @fixme */
		CPlayer Player(PlayerSpec);
		Player.SetPosition(-0.280f, -0.410f);
		FTransformComponent& TransformComp = Player.GetTransformComponent();
		TransformComp.SetTranslation({ -0.28f, -0.41f });
		TransformComp.SetScale({ 0.10f, 0.10f });
		glm::vec3& PlayerPos = TransformComp.Translation;
		glm::vec3& PlayerScale = TransformComp.Scale;

		Player.OnJumped.Add([](const FPlayerData& PlayerData)
		{
			LK_INFO("Player {} jumped", PlayerData.ID);
		});

		glm::vec4 ClearColor{ 0.28f, 0.34f, 0.36f, 1.0f };
		CRenderer::SetClearColor(ClearColor);
		glm::vec4 FragColor{ 1.0f, 0.560f, 1.0f, 1.0f };

		while (Running)
		{
			constexpr float DeltaTime = 0.0f;
			Window.BeginFrame();
			CRenderer::BeginFrame();
			CKeyboard::Update();

			ImGui::Text("%s", LK_TEST_NAME);
			ImGui::Text("Resolution: %dx%d", WindowData.Width, WindowData.Height);
			ImGui::Text("Texture: %dx%d", PlayerTexture.GetWidth(), PlayerTexture.GetHeight());

			ImGui::SetNextItemWidth(320.0f);
			ImGui::SliderFloat3("Background", &ClearColor.x, 0.0f, 1.0f, "%.2f");
			if (ImGui::IsItemActive())
			{
				CRenderer::SetClearColor(ClearColor);
			}

			if (ImGui::Button("Player Jump"))
			{
				LK_TRACE("Button: Player Jump");
				Player.Jump();
			}

			UI_BlendFunction();

			const bool bTable = ImGui::BeginTable("##Table", 2);
			ImGui::TableSetupColumn("##Column1");
			ImGui::TableSetupColumn("##Column2");

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);

			/* -- Player -- */
			ImGui::SeparatorText("Player");
			ImGui::PushID(ImGui::GetID("Player"));
			ImGui::SliderFloat4("Color", &FragColor.x, 0.0f, 1.0f, "%.3f");
			ImGui::SliderFloat2("Position", &PlayerPos.x, -0.50f, 0.50f, "%.2f");
			ImGui::SliderFloat2("Scale", &PlayerScale.x, 0.01f, 0.30f, "%.2f");
			float PlayerRot = glm::degrees(TransformComp.GetRotation2D());
			ImGui::SliderFloat("Rotation", &PlayerRot, -180.0f, 180.0f, "%1.f", ImGuiSliderFlags_ClampOnInput);
			if (ImGui::IsItemActive())
			{
				TransformComp.SetRotation2D(glm::radians(PlayerRot));
			}
			ImGui::PopID();

			static bool bUseCameraProj = false;
			ImGui::Checkbox("Use Camera Projection", &bUseCameraProj);

			static bool bRendererDrawQuad = false;
			ImGui::Checkbox("Renderer: Draw Quad", &bRendererDrawQuad);

			Player.Tick(DeltaTime);
			/* -- ~Player-- */

			ImGui::TableSetColumnIndex(1);

			/* -- Platform -- */
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

			/****************************
			 * Draw platform
			 ****************************/
			glm::mat4 PlatformTransform = glm::mat4(1.0f);
			PlatformTransform = glm::translate(PlatformTransform, glm::vec3(PlatformPos, 0.0f))
				* glm::rotate(PlatformTransform, glm::radians(PlatformRot), glm::vec3(0.0f, 0.0f, 1.0f))
				* glm::scale(PlatformTransform, glm::vec3(PlatformScale, 1.0f));

			Shader.Set("u_transform", PlatformTransform);
			PlatformTexture.Bind();
			Shader.Set("u_texture", 0);
			Shader.Set("u_color", PlatformFragColor);
			glBindVertexArray(RectangleVAO);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			PlatformTexture.Unbind();
			glBindVertexArray(0);

			/****************************
			 * Draw player
			 ****************************/
			glm::mat4 PlayerTransform = TransformComp.GetTransform();
			if (bUseCameraProj)
			{
				const glm::mat4 CameraProj = glm::ortho(0.0f, static_cast<float>(WindowData.Width), 0.0f, static_cast<float>(WindowData.Height), -1.0f, 1.0f);
				PlayerTransform = CameraProj * PlayerTransform;
			}

			PlayerTexture.Bind();
			if (bRendererDrawQuad)
			{
				CRenderer::DrawQuad(Player.GetPosition(), {0.20f, 0.15f}, {0.50f, 0.50f, 0.50f, 1.0f});
			}
			else
			{
				Shader.Set("u_transform", PlayerTransform);
				Shader.Set("u_texture", 0);
				Shader.Set("u_color", FragColor);
				glBindVertexArray(RectangleVAO);
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			}
			PlayerTexture.Unbind();

			ImGui::EndTable();

			CKeyboard::TransitionPressedKeys();
			CRenderer::EndFrame();
			Window.EndFrame();
		}
	}

	void CTest::Destroy()
	{
		glfwTerminate();
	}

	void UI_BlendFunction()
	{
		static constexpr float ItemWidth = 380.0f;
		bool bSetBlendFunc = false;

		static int SelectedSourceBlendFunc = 0;
		static std::pair<const char*, GLenum> SourceBlendFuncs[] = { 
			{ "GL_SRC_ALPHA", GL_SRC_ALPHA },
			{ "GL_DST_ALPHA", GL_DST_ALPHA },
			{ "GL_ONE_MINUS_SRC_ALPHA", GL_ONE_MINUS_SRC_ALPHA },
			{ "GL_ONE_MINUS_CONSTANT_ALPHA", GL_ONE_MINUS_CONSTANT_ALPHA },
		};
		ImGui::SetNextItemWidth(ItemWidth);
		if (ImGui::BeginCombo("Source", SourceBlendFuncs[SelectedSourceBlendFunc].first))
		{
			for (int N = 0; N < LK_ARRAYSIZE(SourceBlendFuncs); N++)
			{
				const bool bSelected = (SelectedSourceBlendFunc == N);
				if (ImGui::Selectable(SourceBlendFuncs[N].first, bSelected))
				{
					SelectedSourceBlendFunc = N;
					LK_INFO("Source: {}", SourceBlendFuncs[N].first);
					bSetBlendFunc = true;
				}
			}
			ImGui::EndCombo();
		}

		static int SelectedDestBlendFunc = 0;
		static std::pair<const char*, GLenum> DestBlendFuncs[] = { 
			{ "GL_ONE_MINUS_SRC_ALPHA", GL_ONE_MINUS_SRC_ALPHA },
			{ "GL_SRC_ALPHA", GL_SRC_ALPHA },
			{ "GL_DST_ALPHA", GL_DST_ALPHA },
			{ "GL_ONE_MINUS_DST_ALPHA", GL_ONE_MINUS_DST_ALPHA },
			{ "GL_ONE_MINUS_CONSTANT_ALPHA", GL_ONE_MINUS_CONSTANT_ALPHA },
		};
		ImGui::SetNextItemWidth(ItemWidth);
		if (ImGui::BeginCombo("Destination", DestBlendFuncs[SelectedDestBlendFunc].first))
		{
			for (int N = 0; N < LK_ARRAYSIZE(DestBlendFuncs); N++)
			{
				const bool bSelected = (SelectedDestBlendFunc == N);
				if (ImGui::Selectable(DestBlendFuncs[N].first, bSelected))
				{
					SelectedDestBlendFunc = N;
					LK_INFO("Destination: {}", DestBlendFuncs[N].first);
					bSetBlendFunc = true;
				}
			}
			ImGui::EndCombo();
		}

		if (bSetBlendFunc)
		{
			glBlendFunc(
				SourceBlendFuncs[SelectedSourceBlendFunc].second, 
				DestBlendFuncs[SelectedDestBlendFunc].second
			);
		}
	}

}