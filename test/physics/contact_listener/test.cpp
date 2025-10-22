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
#include "renderer/debugrenderer.h"
#include "renderer/vertexbufferlayout.h"

#include "game/player.h"
#include "core/input/keyboard.h"
#include "physics/physicsworld.h"
#include "physics/body.h"

#define PLATFORM_ENABLED 1
#define GROUND_PLANE_ENABLED 1

namespace platformer2d::test {

	namespace 
	{
		b2WorldId WorldID;
		bool bPhysicsEnabled = true;
		bool bMetrics = false;
		bool bIdStackTool = false;
		bool bStyleEditor = false;
		bool bBlendFunc = false;
		bool bShowDrawStats = false;
		b2ShapeId PlayerShapeID;
	}

	void UI_MenuBar();
	void UI_ExternalWindows();

	bool PreSolve(b2ShapeId ShapeA, b2ShapeId ShapeB, b2Vec2 Point, b2Vec2 Normal);
	static bool PreSolveStatic(b2ShapeId ShapeA, b2ShapeId ShapeB, b2Vec2 Point, b2Vec2 Normal, void* Ctx)
	{
		return PreSolve(ShapeA, ShapeB, Point, Normal);
	}

	CTest::CTest(const int Argc, char* Argv[])
		: CTestBase(Argc, Argv)
	{
		const glm::vec2 Gravity = { 0.0f, -0.10f };
		CPhysicsWorld::Initialize(Gravity);
		WorldID = CPhysicsWorld::GetWorldID();

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
		FActorSpecification PlayerSpec;
		PlayerSpec.Name = "Player";
		PlayerSpec.Position = { 0.0f, 0.60f };
		PlayerSpec.Size = { 0.40f, 0.20f };

		b2BodyDef PlayerDef = b2DefaultBodyDef();
		PlayerDef.position = { 0.0f, 0.60f };
		PlayerDef.type = b2_dynamicBody;
		PlayerDef.motionLocks.angularZ = true;
		PlayerDef.linearDamping = 1.0f;
		PlayerDef.angularDamping = 1.0f;
		PlayerDef.gravityScale = 1.10f;
		PlayerSpec.BodyDef = PlayerDef;

		PlayerSpec.ShapeDef.enablePreSolveEvents = true;
		PlayerSpec.ShapeDef.material.friction = 0.80f;
		PlayerSpec.ShapeDef.density = 0.30f;
		//PlayerSpec.Shape.Capsule.Radius = 0.14f;
		PlayerSpec.Shape.Capsule.Radius = 0.06f;
		PlayerSpec.ShapeType = EShape_Capsule;

		CPlayer Player(PlayerSpec);
		FTransformComponent& PlayerTC = Player.GetTransformComponent();
		glm::vec3& PlayerPos = PlayerTC.Translation;
		glm::vec3& PlayerScale = PlayerTC.Scale;
		b2World_SetPreSolveCallback(WorldID, PreSolveStatic, &Player /* == Player data */);

		float MovementSpeed = 40.0f; 
		Player.SetMovementSpeed(MovementSpeed);
		Player.OnJumped.Add([](const FPlayerData& PlayerData)
		{
			LK_INFO("Player {} jumped", PlayerData.ID);
		});

		/*********************************
		 * Platform
		 *********************************/
#if PLATFORM_ENABLED
		FActorSpecification PlatformSpec;
		PlatformSpec.Name = "Platform";
		PlatformSpec.Size = { 0.60f, 0.08f };
		PlatformSpec.BodyDef.position = { 0.0f, -0.50 };
		PlatformSpec.BodyDef.type = b2_staticBody;
		PlatformSpec.ShapeDef.enablePreSolveEvents = true;
		PlatformSpec.ShapeType = EShape_Polygon;
		PlatformSpec.Position = { PlatformSpec.BodyDef.position.x, PlatformSpec.BodyDef.position.y };

		CActor Platform(PlatformSpec);
		FTransformComponent& PlatformTC = Platform.GetTransformComponent();
		PlatformTC.SetScale(PlatformSpec.Size);
#endif

		/******************************
		 * TEXTURES
		 ******************************
		 * Player -> Index 1
		 * Platform -> Index 2
		 ******************************/
		auto& Textures = CRenderer::GetTextures();
		std::shared_ptr<CTexture> PlayerTexture = Textures[1];
		std::shared_ptr<CTexture> PlatformTexture = Textures[2];

		glm::vec4 ClearColor{ 0.28f, 0.34f, 0.36f, 1.0f };
		CRenderer::SetClearColor(ClearColor);
		glm::vec4 FragColor{ 1.0f, 0.560f, 1.0f, 1.0f };

#if GROUND_PLANE_ENABLED
		b2BodyDef PlaneDef = b2DefaultBodyDef();
		PlaneDef.position = { 0.0f, -0.60f };
		b2BodyId PlaneID = b2CreateBody(CPhysicsWorld::GetWorldID(), &PlaneDef);
		b2ShapeDef PlaneShapeDef = b2DefaultShapeDef();
		b2Segment PlaneSegment = { { -50.0f, 0.0f }, { 50.0f, 0.0f } };
		b2CreateSegmentShape(PlaneID, &PlaneShapeDef, &PlaneSegment);
#endif

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

			static bool bRendererDrawLine = false;
			ImGui::Checkbox("Draw Line", &bRendererDrawLine);

			ImGui::SameLine(0, 20.0f);
			static bool bRendererDrawCircle = false;
			ImGui::Checkbox("Draw Circle", &bRendererDrawCircle);

#if GROUND_PLANE_ENABLED
			const glm::vec4 PlaneColor = { 1.0f, 0.10f, 0.0f, 1.0f };
			b2Vec2 PlanePos = b2Body_GetPosition(PlaneID);
			const glm::vec3 PlaneP0 = {
				PlanePos.x - (0.50f * PlaneSegment.point2.x),
				PlanePos.y,
				0.0f
			};
			const glm::vec3 PlaneP1 = {
				PlanePos.x + (0.50f * PlaneSegment.point2.x),
				PlanePos.y,
				0.0f
			};
			CRenderer::DrawLine(PlaneP0, PlaneP1, PlaneColor, 3);
#endif

			ImGui::Dummy(ImVec2(0, 12));
			ImGui::SeparatorText("Physics");
			ImGui::Text("Physics");
			ImGui::SameLine();
			ImGui::Checkbox("##Physics", &bPhysicsEnabled);
			if (bPhysicsEnabled)
			{
				CPhysicsWorld::Update(DeltaTime);
			}
			ImGui::SameLine(0, 14.0f);
			if (ImGui::Button("World Step")) CPhysicsWorld::Update(DeltaTime);
			ImGui::SameLine(0, 20.0f);
			const b2Vec2 G = b2World_GetGravity(WorldID);
			ImGui::Text("Gravity: (%.1f, %.1f)", G.x, G.y);

			CDebugRenderer::DrawQuad({ 0.05f, 0.04f }, { 0.10f, 0.20f }, { 0.60f, 0.45f, 0.32f, 0.80f });

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
			Player.Tick(DeltaTime);
			ImGui::SeparatorText("Player");
			ImGui::PushID(ImGui::GetID("Player"));
			ImGui::SliderFloat4("Color", &FragColor.x, 0.0f, 1.0f, "%.3f");
			ImGui::SliderFloat2("Position", &PlayerPos.x, -100.0f, 100.0f, "%.2f");
			if (ImGui::IsItemActive())
			{
				Player.GetBody().SetLinearVelocity({ 0.0f, 0.0f });
				Player.SetPosition({ PlayerTC.Translation.x, PlayerTC.Translation.y });
			}
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
			const CBody& PlayerBody = Player.GetBody();
			const glm::vec2 PlayerBodyPos = PlayerBody.GetPosition();
			ImGui::Text("Body Pos: (%.2f, %.2f)", PlayerBodyPos.x, PlayerBodyPos.y);
			const glm::vec2 PlayerLinearVelocity = PlayerBody.GetLinearVelocity();
			ImGui::Text("Body Linear Velocity: (%.2f, %.2f)", PlayerLinearVelocity.x, PlayerLinearVelocity.y);
			ImGui::PopID();
			/* -- ~Player -- */

			ImGui::TableSetColumnIndex(1);

#if PLATFORM_ENABLED
			Platform.Tick(DeltaTime);

			/* -- Platform -- */
			static glm::vec4 PlatformFragColor{ 1.0f, 1.0f, 1.0f, 1.0f };
			ImGui::SeparatorText("Platform");
			ImGui::PushID(ImGui::GetID("Platform"));
			ImGui::SliderFloat4("Color", &PlatformFragColor.x, 0.0f, 1.0f, "%.3f");
			ImGui::SliderFloat2("Position", &PlatformTC.Translation.x, -2.0, 2.0f, "%.2f");
			if (ImGui::IsItemActive())
			{
				Platform.SetPosition({ PlatformTC.Translation.x, PlatformTC.Translation.y });
			}
			ImGui::SliderFloat2("Scale", &PlatformTC.Scale.x, 0.01f, 2.0f, "%.2f");
			float PlatformRot = glm::degrees(PlatformTC.GetRotation2D());
			ImGui::SliderFloat("Rotation", &PlayerRot, -180.0f, 180.0f, "%1.f", ImGuiSliderFlags_ClampOnInput);
			if (ImGui::IsItemActive())
			{
				PlatformTC.SetRotation2D(glm::radians(PlatformRot));
			}
			ImGui::PopID();

			glm::mat4 PlatformTransform = PlatformTC.GetTransform();
			CRenderer::DrawQuad(Platform.GetPosition(), PlatformTC.Scale, *PlatformTexture, PlatformFragColor);
#endif

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

	bool PreSolve(b2ShapeId ShapeA, b2ShapeId ShapeB, b2Vec2 Point, b2Vec2 Normal)
	{
		LK_ASSERT(b2Shape_IsValid(ShapeA));
		LK_ASSERT(b2Shape_IsValid(ShapeB));

		float Sign = 0.0f;
		if (B2_ID_EQUALS(ShapeA, PlayerShapeID))
		{
			Sign = -1.0f;
		}
		else if (B2_ID_EQUALS(ShapeB, PlayerShapeID))
		{
			Sign = 1.0f;
		}
		else
		{
			/* Not colliding with the player, enable contact. */
			return true;
		}

		if ((Sign * Normal.y) > 0.95f)
		{
			return true;
		}

		/* Normal points down, disable contact. */
		return false;
	}

}