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
#include "core/input/mouse.h"
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

		bool bRendererDrawLine = false;
		bool bRendererDrawCircle = false;
		bool bRendererDrawCapsule = false;

		std::shared_ptr<CCamera> Camera = nullptr;
		constexpr float FAR_PLANE  =  1.0f;
		constexpr float NEAR_PLANE = -1.0f;
	}

	void UI_MenuBar();
	void UI_ExternalWindows();
	void UI_CircleDrawMenu(const CPlayer& Player);

	bool PreSolve(b2ShapeId ShapeA, b2ShapeId ShapeB, b2Vec2 Point, b2Vec2 Normal);
	static bool PreSolveStatic(b2ShapeId ShapeA, b2ShapeId ShapeB, b2Vec2 Point, b2Vec2 Normal, void* Ctx)
	{
		return PreSolve(ShapeA, ShapeB, Point, Normal);
	}

	CTest::CTest(const int Argc, char* Argv[])
		: CTestBase(Argc, Argv)
	{
		const glm::vec2 Gravity = { 0.0f, -3.20f };
		CPhysicsWorld::Initialize(Gravity);
		WorldID = CPhysicsWorld::GetID();

		CRenderer::Initialize();
		CKeyboard::Initialize();
		CMouse::Initialize();
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

		Camera = std::make_shared<CCamera>(SCREEN_WIDTH, SCREEN_HEIGHT, NEAR_PLANE, FAR_PLANE);

		/*********************************
		 * Player
		 *********************************/
		FCapsule PlayerCapsule;
		PlayerCapsule.P0 = { 0.0f, 0.0f };
		PlayerCapsule.P1 = { 0.0f, 0.20f };
		PlayerCapsule.Radius = 0.10f;

		FBodySpecification BodySpec;
		BodySpec.Type = EBodyType::Dynamic;
		BodySpec.Position = { 0.0f, 1.0f };
		BodySpec.Friction = 0.10f;
		BodySpec.Density = 0.60f;
		BodySpec.LinearDamping = 0.50f;
		BodySpec.MotionLock = EMotionLock_Z;
		BodySpec.Shape.emplace<FCapsule>(PlayerCapsule);
		CPlayer Player(BodySpec);

		const FPlayerData& PlayerData = Player.GetData();
		FTransformComponent& PlayerTC = Player.GetTransformComponent();
		glm::vec3& PlayerPos = PlayerTC.Translation;
		glm::vec3& PlayerScale = PlayerTC.Scale;
		const CBody& PlayerBody = Player.GetBody();
		b2World_SetPreSolveCallback(WorldID, PreSolveStatic, &Player /* == Player data */);

		Player.OnJumped.Add([](const FPlayerData& PlayerData)
		{
			LK_TRACE("Player {} jumped", PlayerData.ID);
		});

		Player.OnLanded.Add([](const FPlayerData& PlayerData)
		{
			LK_TRACE("Player {} landed", PlayerData.ID);
		});

#if PLATFORM_ENABLED
		FBodySpecification PlatformSpec;
		PlatformSpec.Position = { 0.0f, -0.80 };
		PlatformSpec.Type = EBodyType::Static;
		PlatformSpec.Flags = EBodyFlag_PreSolveEvents;

		FPolygon PlatformPolygon = {
			.Size = { 2.0f, 0.08f }
		};
		PlatformSpec.Shape.emplace<FPolygon>(PlatformPolygon);

		CActor Platform(PlatformSpec);
		FTransformComponent& PlatformTC = Platform.GetTransformComponent();
		PlatformTC.SetScale(PlatformPolygon.Size);
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

		b2BodyId PlaneID;
		constexpr float HalfW = 12.0f;
		constexpr float HalfH = 0.10f;
#if GROUND_PLANE_ENABLED
		{
			b2BodyDef PlaneDef = b2DefaultBodyDef();
			PlaneDef.type = b2_staticBody;
			PlaneDef.position = { 0.0f, -0.60f };
			PlaneID = b2CreateBody(CPhysicsWorld::GetID(), &PlaneDef);
			b2ShapeDef PlaneShapeDef = b2DefaultShapeDef();
			PlaneShapeDef.enablePreSolveEvents = true;
			b2Polygon PlaneBox = b2MakeBox(HalfW, HalfH);
			b2CreatePolygonShape(PlaneID, &PlaneShapeDef, &PlaneBox);
		}
#endif

		b2BodyId SmallPlatformID;
		constexpr float SmallPlatformHalfW = 0.20f;
		constexpr float SmallPlatformHalfH = 0.03f;
		{
			b2BodyDef SmallPlatformDef = b2DefaultBodyDef();
			SmallPlatformDef.type = b2_staticBody;
			SmallPlatformDef.position = { -0.43f, 0.14f };
			SmallPlatformID = b2CreateBody(CPhysicsWorld::GetID(), &SmallPlatformDef);
			b2ShapeDef SmallPlatformShapeDef = b2DefaultShapeDef();
			SmallPlatformShapeDef.enablePreSolveEvents = true;
			b2Polygon SmallPlatformBox = b2MakeBox(SmallPlatformHalfW, SmallPlatformHalfH);
			b2CreatePolygonShape(SmallPlatformID, &SmallPlatformShapeDef, &SmallPlatformBox);
		}

		Timer.Reset();
		while (bRunning)
		{
			const float DeltaTime = Timer.GetDeltaTime();

			Window.BeginFrame();
			CKeyboard::Update();
			CRenderer::BeginFrame();

			Camera->SetViewportSize(SCREEN_WIDTH, SCREEN_HEIGHT);
			CRenderer::BeginScene(*Camera);

			UI_MenuBar();
			ImGui::Text("%s", LK_TEST_NAME);
			ImGui::Text("Resolution: %dx%d", WindowData.Width, WindowData.Height);
			ImGui::Text("Delta Time: %.3fms", DeltaTime);

			ImGui::Checkbox("Draw Line", &bRendererDrawLine);

			ImGui::SameLine(0, 20.0f);
			ImGui::Checkbox("Draw Circle", &bRendererDrawCircle);

			ImGui::SameLine(0, 20.0f);
			ImGui::Checkbox("Draw Capsule", &bRendererDrawCapsule);
			if (bRendererDrawCapsule)
			{
				CDebugRenderer::DrawCapsule(PlayerCapsule.P0, PlayerCapsule.P1, PlayerCapsule.Radius, FColor::Green);
			}

			ImGui::SameLine(0, 40.0f);
			float CameraZoom = Camera->GetZoom();
			ImGui::SetNextItemWidth(300.0f);
			ImGui::SliderFloat("Camera Zoom", &CameraZoom, CCamera::ZOOM_MIN, CCamera::ZOOM_MAX, "%.2f");
			if (ImGui::IsItemActive())
			{
				Camera->SetZoom(CameraZoom);
			}

			ImGui::SameLine(0, 40.0f);
			const auto [MousePosX, MousePosY] = CMouse::GetPos();
			ImGui::Text("Mouse Pos: (%.2f, %.2f)", MousePosX, MousePosY);

#if GROUND_PLANE_ENABLED
			{
				const glm::vec4& PlaneColor = FragColor;
				const b2Vec2 PlanePos = b2Body_GetPosition(PlaneID);
				const glm::vec2 Pos  = { PlanePos.x, PlanePos.y };
				const glm::vec2 Size = { 2.0f * HalfW, 2.0f * HalfH };
				CRenderer::DrawQuad(Pos, Size, PlaneColor, 0.0f);
			}
#endif
			{
				const glm::vec4& SmallPlatformColor = FragColor;
				const b2Vec2 SmallPlatformBodyPos = b2Body_GetPosition(SmallPlatformID);
				const glm::vec2 Pos = { SmallPlatformBodyPos.x, SmallPlatformBodyPos.y };
				const glm::vec2 Size = { 2.0f * SmallPlatformHalfW, 2.0f * SmallPlatformHalfH };
				CRenderer::DrawQuad(Pos, Size, SmallPlatformColor, 0.0f);
			}


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

			ImGui::SetNextItemWidth(310.0f);
			ImGui::SliderFloat3("Background", &ClearColor.x, 0.0f, 1.0f, "%.2f");
			if (ImGui::IsItemActive()) CRenderer::SetClearColor(ClearColor);

			const bool bTable = ImGui::BeginTable("##Table", 2);
			ImGui::TableSetupColumn("##Column1");
			ImGui::TableSetupColumn("##Column2");

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);

			/* -- Player -- */
			Player.Tick(DeltaTime);
			ImGui::PushItemWidth(200.0f);
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
			ImGui::PopItemWidth();
			const glm::vec2 PlayerBodyPos = PlayerBody.GetPosition();
			const glm::vec2 PlayerLinearVelocity = PlayerBody.GetLinearVelocity();
			ImGui::Text("Body Pos: (%.2f, %.2f)", PlayerBodyPos.x, PlayerBodyPos.y);
			ImGui::Text("Body Linear Velocity: (%.2f, %.2f)", PlayerLinearVelocity.x, PlayerLinearVelocity.y);

			ImGui::PushItemWidth(200.0f);
			static float PlayerJumpImpulse = Player.GetJumpImpulse();
			ImGui::SliderFloat("Jump Impulse", &PlayerJumpImpulse, 0.0f, 10.0f, "%.5f");
			if (ImGui::IsItemActive()) Player.SetJumpImpulse(PlayerJumpImpulse);
			static float PlayerDirForce = Player.GetDirectionForce();
			ImGui::SliderFloat("Direction Force", &PlayerDirForce, 0.0f, 10.0f, "%.5f");
			if (ImGui::IsItemActive()) Player.SetDirectionForce(PlayerDirForce);
			ImGui::Text("Body Pos: (%.2f, %.2f)", PlayerBodyPos.x, PlayerBodyPos.y);
			ImGui::Text("Player jumping: %s", PlayerData.bJumping ? "True" : "False");

			float Mass = PlayerBody.GetMass();
			ImGui::SliderFloat("Mass", &Mass, 0.0f, 10.0f, "%.2f");
			if (ImGui::IsItemActive()) PlayerBody.SetMass(Mass);

			static float BodyScale = 1.0f;
			ImGui::SliderFloat("Body Scale", &BodyScale, 0.0f, 2.0f, "%.2f");
			ImGui::SameLine();
			if (ImGui::Button("Apply##Scale")) PlayerBody.SetScale(BodyScale);
			ImGui::PopItemWidth();
			ImGui::PopID();
			/* -- ~Player -- */

			ImGui::TableSetColumnIndex(1);

#if PLATFORM_ENABLED
			Platform.Tick(DeltaTime);
			static glm::vec4 PlatformFragColor{ 1.0f, 1.0f, 1.0f, 1.0f };
			glm::mat4 PlatformTransform = PlatformTC.GetTransform();
			CRenderer::DrawQuad(Platform.GetPosition(), PlatformTC.Scale, *PlatformTexture, PlatformFragColor);
#endif

			/* Draw player */
			glm::mat4 PlayerTransform = PlayerTC.GetTransform();
			static glm::vec2 PlayerSize = { 0.20f, 0.20f };
			CRenderer::DrawQuad(Player.GetPosition(), PlayerSize, *PlayerTexture, FragColor);

			ImGui::EndTable();

			ImGui::PushItemWidth(240.0f);
			static float CameraProj = 1.0f;
			ImGui::SliderFloat("Camera Projection", &CameraProj, 0.10f, 1.0f);
			if (ImGui::IsItemActive())
			{
				const glm::mat4 ViewProj(CameraProj);
				CRenderer::SetCameraViewProjection(ViewProj);
			}
			ImGui::PopItemWidth();

			UI_ExternalWindows();
			UI_CircleDrawMenu(Player);

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

	void UI_CircleDrawMenu(const CPlayer& Player)
	{
		ImGui::PushItemWidth(260.0f);
		static float Radius = 0.15f;
		static float FillRadius = 0.15f;
		static float FillThickness = 1.0f;
		static glm::vec4 CircleColor = { 0.25f, 0.90f, 0.10f, 0.60f };
		ImGui::SliderFloat("Circle Radius", &Radius, 0.001f, 1.0f);
		ImGui::SliderFloat("Circle Fill Radius", &FillRadius, 0.01f, 1.50f);
		ImGui::SliderFloat("Circle Fill Thickness", &FillThickness, 0.001f, 1.0f);
		ImGui::SliderFloat4("Circle Color", &CircleColor.x, 0.0f, 1.0f, "%.3f");
		ImGui::PopItemWidth();
		if (bRendererDrawCircle)
		{
			glm::vec3 Rot = glm::vec3(0.0f);
			CRenderer::DrawCircle(Player.GetPosition(), Rot, Radius, { 0.30f, 1.0f, 0.50f, 1.0f });
			CRenderer::DrawCircleFilled(Player.GetPosition(), FillRadius, CircleColor, FillThickness);
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