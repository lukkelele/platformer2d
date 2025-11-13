#include "test.h"

#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_common.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/norm.hpp>
#include <spdlog/spdlog.h>
#include <stb/stb_image.h>
#include <box2d/box2d.h>

#include "core/assert.h"
#include "core/window.h"
#include "core/timer.h"
#include "renderer/renderer.h"
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

		b2WorldId WorldID;

		bool bMetrics = false;
		bool bIdStackTool = false;
		bool bStyleEditor = false;
		bool bBlendFunc = false;
		bool bShowDrawStats = false;
	}

	void UI_MenuBar();

	CTest::CTest(const int Argc, char* Argv[])
		: CTestBase(Argc, Argv)
	{
		CRenderer::Initialize();
		CKeyboard::Initialize();
	}

	void CTest::Run()
	{
		bRunning = true;
		const int CatchResult = Catch::Session().run(Args.Argc, Args.Argv);
		LK_DEBUG("Catch result: {}", CatchResult);

		const std::filesystem::path& BinaryDir = GetBinaryDirectory();
		CWindow& Window = GetWindow();
		const FWindowData& WindowData = Window.GetData();
		CTimer Timer;

		/*********************************
		 * Rectangle
		 *********************************/
		const FVertexBufferLayout RectangleLayout = {
			{ "pos",      EShaderDataType::Float2, },
			{ "texcoord", EShaderDataType::Float2, },
		};

		GLuint RectangleVAO = OpenGL::VertexArray::Create();
		GLuint RectangleVBO = OpenGL::VertexBuffer::Create(RectangleVertices, RectangleLayout);
		GLuint RectangleEBO = OpenGL::ElementBuffer::Create(RectangleIndices);
		glBindVertexArray(0);

		/*********************************
		 * Shader
		 *********************************/
		CShader Shader(BinaryDir / "player.shader");
		CShader LineShader(BinaryDir / "line.shader");

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
		PlayerTexture.SetSlot(1);
		LK_INFO("PlayerTexture Index: {}", PlayerTexture.GetSlot());

		Spec.Path = BricksTexturePath;
		Spec.Width = 512;
		Spec.Height = 512;
		CTexture PlatformTexture(Spec);
		LK_INFO("PlatformTexture Index: {}", PlatformTexture.GetSlot());

		/*********************************
		 * Player
		 *********************************/
		CPlayer Player;
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

		/*********************************
		 * Line rendering
		 *********************************/
		GLuint LineVAO;
		GLuint LineVBO;
		glGenVertexArrays(1, &LineVAO);
		glGenBuffers(1, &LineVBO);

		glBindVertexArray(LineVAO);
		glBindBuffer(GL_ARRAY_BUFFER, LineVBO);
		glBufferData(GL_ARRAY_BUFFER, 2 * sizeof(glm::vec2), nullptr, GL_DYNAMIC_DRAW);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), nullptr);
		glEnableVertexAttribArray(0);
		
		/*********************************
		 * Physics
		 *********************************/
		b2WorldDef WorldDef = b2DefaultWorldDef();
		WorldDef.gravity = b2Vec2(0.0f, -10.0f);
		WorldID = b2CreateWorld(&WorldDef);
		LK_INFO("WorldID.index1: {}", static_cast<int>(WorldID.index1));
		LK_INFO("WorldID.generation: {}", static_cast<int>(WorldID.generation));

		/* Create ground body. */
		b2BodyDef GroundBodyDef = b2DefaultBodyDef();
		b2BodyId GroundID = b2CreateBody(WorldID, &GroundBodyDef);
		b2Polygon GroundBox = b2MakeBox(50.0f, 10.0f);
		b2ShapeDef GroundShapeDef = b2DefaultShapeDef();
		b2CreatePolygonShape(GroundID, &GroundShapeDef, &GroundBox);

		/* Dynamic body. */
		b2BodyDef DynamicBodyDef = b2DefaultBodyDef();
		DynamicBodyDef.type = b2BodyType::b2_dynamicBody;
		DynamicBodyDef.position = { 0.0f, 4.0f };
		b2BodyId DynamicBodyID = b2CreateBody(WorldID, &DynamicBodyDef);
		LK_INFO("DynamicBodyID.world0: {}", DynamicBodyID.world0);
		b2Polygon DynamicBox = b2MakeBox(1.0f, 1.0f);
		LK_INFO("DynamicBox.count: {}", DynamicBox.count);
		b2ShapeDef DynamicShapeDef = b2DefaultShapeDef();
		DynamicShapeDef.density = 1.0f;
		DynamicShapeDef.material.friction = 0.30f;
		const b2ShapeId DynamicShapeID = b2CreatePolygonShape(DynamicBodyID, &DynamicShapeDef, &DynamicBox);

		int SubStepCount = 4;

		Timer.Reset();
		while (bRunning)
		{
			const float DeltaTime = Timer.GetDeltaTime();
			Window.BeginFrame();
			CKeyboard::Update();
			CRenderer::BeginFrame();

			UI_MenuBar();
			ImGui::Text("%s", LK_TEST_NAME);
			ImGui::Text("Resolution: %dx%d", WindowData.Width, WindowData.Height);
			ImGui::Text("Delta Time: ~%1.fms", DeltaTime);
			ImGui::Text("Texture: %dx%d", PlayerTexture.GetWidth(), PlayerTexture.GetHeight());

			static bool bVSync = Window.GetVSync();
			if (ImGui::Checkbox("VSync", &bVSync)) 
			{
				Window.SetVSync(bVSync);
			}

			ImGui::SameLine(0, 20.0f);
			static bool bRendererDrawQuad = false;
			ImGui::Checkbox("Draw Quad", &bRendererDrawQuad);

			ImGui::SameLine(0, 20.0f);
			static bool bRendererDrawLine = false;
			ImGui::Checkbox("Submit Line", &bRendererDrawLine);

			ImGui::SameLine(0, 20.0f);
			static bool bRendererDrawCircle = false;
			ImGui::Checkbox("Draw Circle", &bRendererDrawCircle);

			const FDrawStatistics& DrawStats = CRenderer::GetDrawStatistics();
			if (ImGui::TreeNodeEx("Draw Statistics", ImGuiTreeNodeFlags_SpanLabelWidth))
			{
				ImGui::Text("Quads: %ld", DrawStats.QuadCount);
				ImGui::Text("Lines: %ld", DrawStats.LineCount);

				ImGui::TreePop();
			}

			ImGui::Dummy(ImVec2(0, 12));
			ImGui::SeparatorText("Physics");
			static bool bPhysicsEnabled = false;
			ImGui::Text("Physics");
			ImGui::SameLine();
			ImGui::Checkbox("##Physics", &bPhysicsEnabled);
			if (bPhysicsEnabled)
			{
				b2World_Step(WorldID, DeltaTime, SubStepCount);
			}
			ImGui::SameLine(0, 20.0f);
			if (ImGui::Button("World Step"))
			{
				b2World_Step(WorldID, DeltaTime, SubStepCount);
			}
			ImGui::SameLine(0, 20.0f);
			ImGui::Text("Substep Count");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(110.0f);
			ImGui::SliderInt("##SubstepCount", &SubStepCount, 1, 20);

			ImGui::Text("Dynamic Body ID: %d", DynamicBodyID.index1);
			const b2Vec2 DynamicBodyPos = b2Body_GetPosition(DynamicBodyID);
			const b2Rot DynamicBodyRot = b2Body_GetRotation(DynamicBodyID);
			ImGui::Text("Position: (%4.2f, %4.2f)", DynamicBodyPos.x, DynamicBodyPos.y);
			ImGui::Text("Rotation: %4.2f (rad)", b2Rot_GetAngle(DynamicBodyRot));
			ImGui::Dummy(ImVec2(0, 16));

			ImGui::SetNextItemWidth(310.0f);
			ImGui::SliderFloat3("Background", &ClearColor.x, 0.0f, 1.0f, "%.2f");
			if (ImGui::IsItemActive())
			{
				CRenderer::SetClearColor(ClearColor);
			}

			ImGui::SameLine();
			if (ImGui::Button("Player Jump"))
			{
				Player.Jump();
			}

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
			ImGui::Text("Player Texture: %ld", PlayerTexture.GetSlot());
			ImGui::PopID();

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
			PlatformTexture.Unbind(0);
			glBindVertexArray(0);

			/****************************
			 * Draw player
			 ****************************/
			glm::mat4 PlayerTransform = TransformComp.GetTransform();
			static glm::vec2 PlayerSize = { 0.20f, 0.20f };
			if (bRendererDrawQuad)
			{
				CRenderer::DrawQuad(Player.GetPosition(), PlayerSize, PlayerTexture, FragColor);
			}
			else
			{
				const std::size_t PlayerTexIndex = PlayerTexture.GetSlot();
				PlayerTexture.Bind(PlayerTexIndex);
				Shader.Set("u_transform", PlayerTransform);
				Shader.Set("u_texture", 1);
				Shader.Set("u_color", FragColor);
				glBindVertexArray(RectangleVAO);
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
				PlayerTexture.Unbind(PlayerTexIndex);
			}

			ImGui::EndTable();

			/* Draw line. */
			ImGui::SeparatorText("Line");
			static glm::vec2 Start = { -0.50f, 0.39f };
			static glm::vec2 End = { 0.50f,  0.50f };
			ImGui::SetNextItemWidth(250.0f);
			const bool bLineStart = ImGui::SliderFloat2("Start", &Start.x, -10.0f, 10.0f, "%.2f");
			ImGui::SetNextItemWidth(250.0f);
			const bool bLineEnd = ImGui::SliderFloat2("End", &End.x, -10.0f, 10.0f, "%.2f");

			const glm::vec4 LineColor = { 1.0f, 0.50f, 1.0f, 1.0f };
			static int LineWidth = 8;
			ImGui::SetNextItemWidth(190.0f);
			ImGui::SliderInt("Line Width", &LineWidth, 1, 24);
			if (bRendererDrawLine)
			{
				CRenderer::DrawLine(Start, End, LineColor, LineWidth);

				const glm::vec2 LeftPos = { PlayerPos.x - (PlayerSize.x * 0.50f), PlayerPos.y };
				const glm::vec2 RightPos = { PlayerPos.x + (PlayerSize.x * 0.50f), PlayerPos.y};
				CRenderer::DrawLine(LeftPos, RightPos, LineColor, LineWidth);
			}
			else
			{
				LineShader.Bind();
				LineShader.Set("u_transform", glm::mat4(1.0f));
				LineShader.Set("u_color", { 1.0f, 0.50f, 1.0f, 1.0f });
				glBindBuffer(GL_ARRAY_BUFFER, LineVBO);
				glm::vec2 Vertices[2] = { Start, End };
				glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertices), Vertices);
				glBindVertexArray(LineVAO);
				glLineWidth(LineWidth);
				glDrawArrays(GL_LINES, 0, 2);
				LineShader.Unbind();
			}

			if (bIdStackTool)
			{
				ImGui::ShowIDStackToolWindow(&bIdStackTool);
			}
			if (bMetrics)
			{
				ImGui::ShowMetricsWindow(&bMetrics);
			}
			if (bStyleEditor && ImGui::Begin("Style Editor", &bStyleEditor))
			{
				ImGuiStyle& Style = ImGui::GetStyle();
				ImGui::ShowStyleEditor(&Style);
				ImGui::End();
			}
			if (bBlendFunc && ImGui::Begin("Blend Function", &bBlendFunc))
			{
				CTest::UI_BlendFunction();
				ImGui::End();
			}

			static float Radius = 0.15f;
			static float FillRadius = 0.15f;
			static float FillThickness = 1.0f;
			static glm::vec4 CircleColor = { 0.25f, 0.90f, 0.10f, 0.60f };
			ImGui::SetNextItemWidth(100.0f);
			ImGui::SliderFloat("Circle Radius", &Radius, 0.001f, 1.0f);
			ImGui::SetNextItemWidth(100.0f);
			ImGui::SliderFloat("Circle Fill Radius", &FillRadius, 0.01f, 1.50f);
			ImGui::SetNextItemWidth(100.0f);
			ImGui::SliderFloat("Circle Fill Thickness", &FillThickness, 0.001f, 1.0f);
			ImGui::SetNextItemWidth(290.0f);
			ImGui::SliderFloat4("Circle Color", &CircleColor.x, 0.0f, 1.0f, "%.3f");
			if (bRendererDrawCircle)
			{
				glm::vec3 Rot = glm::vec3(0.0f);
				CRenderer::DrawCircle(Player.GetPosition(), Rot, Radius, { 0.30f, 1.0f, 0.50f, 1.0f });
				CRenderer::DrawCircleFilled(Player.GetPosition(), FillRadius, CircleColor, FillThickness);
			}

			CRenderer::Flush();
			CRenderer::EndFrame();
			CKeyboard::TransitionPressedKeys();
			Window.EndFrame();
		}
	}

	void CTest::Destroy()
	{
		b2DestroyWorld(WorldID);
		glfwTerminate();
	}

	void UI_MenuBar()
	{
		if (!ImGui::BeginMenuBar())
		{
			return;
		}

		if (ImGui::BeginMenu("Settings"))
		{
			ImGui::Checkbox("Style Editor", &bStyleEditor);
			if (ImGui::MenuItem("ID Stack Tool", nullptr))
			{
				bIdStackTool = !bIdStackTool;
			}
			if (ImGui::MenuItem("Metrics", nullptr))
			{
				bMetrics = !bMetrics;
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Renderer"))
		{
			ImGui::Checkbox("Draw Statistics", &bShowDrawStats);
			if (ImGui::MenuItem("Blend Function", nullptr))
			{
				bBlendFunc = !bBlendFunc;
			}
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

}