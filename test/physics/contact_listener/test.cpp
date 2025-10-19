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
		b2WorldId WorldID;

		bool bMetrics = false;
		bool bIdStackTool = false;
		bool bStyleEditor = false;
		bool bBlendFunc = false;
		bool bShowDrawStats = false;
	}

	void UI_MenuBar();
	void UI_ExternalWindows();

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
		CTimer Timer;

		/*********************************
		 * Player
		 *********************************/
		CPlayer Player("TestPlayer");
		Player.SetPosition(-0.280f, -0.410f);
		FTransformComponent& PlayerTC = Player.GetTransformComponent();
		PlayerTC.SetTranslation({ -0.28f, -0.41f });
		PlayerTC.SetScale({ 0.10f, 0.10f });
		glm::vec3& PlayerPos = PlayerTC.Translation;
		glm::vec3& PlayerScale = PlayerTC.Scale;

		float MovementSpeed = 40.0f; 
		Player.SetMovementSpeed(MovementSpeed);
		Player.OnJumped.Add([](const FPlayerData& PlayerData)
		{
			LK_INFO("Player {} jumped", PlayerData.ID);
		});

		/*********************************
		 * Platform
		 *********************************/
		CActor Platform;
		Platform.SetPosition({ -0.12f, -0.67f });
		FTransformComponent& PlatformTC = Platform.GetTransformComponent();
		//PlatformTC.SetScale({ 0.65f, 0.31f });

		glm::vec4 ClearColor{ 0.28f, 0.34f, 0.36f, 1.0f };
		CRenderer::SetClearColor(ClearColor);
		glm::vec4 FragColor{ 1.0f, 0.560f, 1.0f, 1.0f };

		/******************************
		 * TEXTURES
		 ******************************
		 * Player -> Index 1
		 * Platform -> Index 2
		 ******************************
		 */
		auto& Textures = CRenderer::GetTextures();
		std::shared_ptr<CTexture> PlayerTexture = Textures[1];
		std::shared_ptr<CTexture> PlatformTexture = Textures[2];
		LK_INFO("PlayerTexture Index: {}", PlayerTexture->GetIndex());
		LK_INFO("PlatformTexture Index: {}", PlatformTexture->GetIndex());
		
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
		while (Running)
		{
			const float DeltaTime = Timer.GetDeltaTime();
			Window.BeginFrame();
			CKeyboard::Update();
			CRenderer::BeginFrame();

			UI_MenuBar();
			ImGui::Text("%s", LK_TEST_NAME);
			ImGui::Text("Resolution: %dx%d", WindowData.Width, WindowData.Height);
			ImGui::Text("Delta Time: ~%1.fms", DeltaTime);

			ImGui::SameLine(0, 20.0f);
			static bool bRendererDrawLine = false;
			ImGui::Checkbox("Draw Line", &bRendererDrawLine);

			ImGui::SameLine(0, 20.0f);
			static bool bRendererDrawCircle = false;
			ImGui::Checkbox("Draw Circle", &bRendererDrawCircle);

			const FDrawStatistics& DrawStats = CRenderer::GetDrawStatistics();
			if (ImGui::TreeNodeEx("Draw Statistics", ImGuiTreeNodeFlags_SpanLabelWidth))
			{
				ImGui::Text("Quads: %d", DrawStats.QuadCount);
				ImGui::Text("Lines: %d", DrawStats.LineCount);
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

			ImGui::Text("Dynamic Body ID: %d", DynamicBodyID);
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
			float PlayerRot = glm::degrees(PlayerTC.GetRotation2D());
			ImGui::SliderFloat("Rotation", &PlayerRot, -180.0f, 180.0f, "%1.f", ImGuiSliderFlags_ClampOnInput);
			if (ImGui::IsItemActive())
			{
				PlayerTC.SetRotation2D(glm::radians(PlayerRot));
			}
			ImGui::SliderFloat("Movement Speed", &MovementSpeed, 1.50f, 60.0f, "%.2f");
			if (ImGui::IsItemActive())
			{
				Player.SetMovementSpeed(MovementSpeed);
			}
			ImGui::PopID();

			Player.Tick();
			/* -- ~Player-- */

			ImGui::TableSetColumnIndex(1);

			/* -- Platform -- */
			static glm::vec4 PlatformFragColor{ 1.0f, 1.0f, 1.0f, 1.0f };
			ImGui::SeparatorText("Platform");
			ImGui::PushID(ImGui::GetID("Platform"));
			ImGui::SliderFloat4("Color", &PlatformFragColor.x, 0.0f, 1.0f, "%.3f");
			ImGui::SliderFloat2("Position", &PlatformTC.Translation.x, -2.0, 2.0f, "%.2f");
			ImGui::SliderFloat2("Scale", &PlatformTC.Scale.x, 0.01f, 2.0f, "%.2f");
			float PlatformRot = glm::degrees(PlatformTC.GetRotation2D());
			ImGui::SliderFloat("Rotation", &PlayerRot, -180.0f, 180.0f, "%1.f", ImGuiSliderFlags_ClampOnInput);
			if (ImGui::IsItemActive())
			{
				PlatformTC.SetRotation2D(glm::radians(PlatformRot));
			}
			ImGui::PopID();

			/****************************
			 * Draw platform
			 ****************************/
			glm::mat4 PlatformTransform = PlatformTC.GetTransform();
			static glm::vec2 PlatformSize = { 0.60f, 0.31f };
			CRenderer::DrawQuad(Platform.GetPosition(), PlatformSize, *PlatformTexture, PlatformFragColor);

			/****************************
			 * Draw player
			 ****************************/
			glm::mat4 PlayerTransform = PlayerTC.GetTransform();
			static glm::vec2 PlayerSize = { 0.20f, 0.20f };
			CRenderer::DrawQuad(Player.GetPosition(), PlayerSize, *PlayerTexture, FragColor);

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

			UI_ExternalWindows();

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
			static bool bVSync = CWindow::Get()->GetVSync();
			if (ImGui::Checkbox("VSync", &bVSync)) 
			{
				CWindow::Get()->SetVSync(bVSync);
			}

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

	void UI_ExternalWindows()
	{
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
	}

}