#include <stdio.h>
#include <filesystem>

#include <imgui/imgui.h>
#include <stb/stb_image.h>
#include <spdlog/spdlog.h>

#include "core/assert.h"
#include "core/window.h"
#include "core/timer.h"
#include "core/input/keyboard.h"
#include "core/input/mouse.h"
#include "game/player.h"
#include "renderer/renderer.h"
#include "renderer/texture.h"
#include "physics/physicsworld.h"

using namespace platformer2d;

namespace
{
	bool bPhysicsEnabled = true;
	bool bMetrics = false;
	bool bIdStackTool = false;
	bool bStyleEditor = false;
	bool bBlendFunc = false;
	bool bShowDrawStats = false;

	std::vector<std::shared_ptr<CTexture>> Textures;
	std::shared_ptr<CTexture> PlayerTexture = nullptr;
	std::shared_ptr<CTexture> PlatformTexture = nullptr;

	glm::vec2 PlayerSize = { 0.0f, 0.0f };
}

std::unique_ptr<CPlayer> CreatePlayer();
void DrawPlayer(const CPlayer& Player);

std::unique_ptr<CActor> CreatePlatform();
void DrawPlatform(const CActor& Platform);

bool PreSolve(b2ShapeId ShapeA, b2ShapeId ShapeB, b2Vec2 Point, b2Vec2 Normal, const CPlayer& Player);
static bool PreSolveStatic(b2ShapeId ShapeA, b2ShapeId ShapeB, b2Vec2 Point, b2Vec2 Normal, void* Ctx)
{
	CPlayer& Player = *static_cast<CPlayer*>(Ctx);
	return PreSolve(ShapeA, ShapeB, Point, Normal, Player);
}

void UI_MenuBar();

int main(int Argc, char* Argv[])
{
	CLog::Initialize();

	CWindow Window(SCREEN_WIDTH, SCREEN_HEIGHT, "platformer2d");
	Window.Initialize();
	const FWindowData& WindowData = Window.GetData();

	const glm::vec2 Gravity = { 0.0f, -3.20f };
	CPhysicsWorld::Initialize(Gravity);

	CTimer Timer; 
	CRenderer::Initialize();
	CKeyboard::Initialize();
	CMouse::Initialize();

	Textures = CRenderer::GetTextures();
	PlayerTexture = Textures[1];
	PlatformTexture = Textures[2];

	std::unique_ptr<CPlayer> Player = CreatePlayer();
	std::unique_ptr<CActor> Platform = CreatePlatform();

	constexpr float FAR_PLANE = 1.0f;
	constexpr float NEAR_PLANE = -1.0f;
	std::shared_ptr<CCamera> Camera = std::make_shared<CCamera>(SCREEN_WIDTH, SCREEN_HEIGHT, NEAR_PLANE, FAR_PLANE);

	Timer.Reset();
	bool bRunning = true;
	while (bRunning)
	{
		const float DeltaTime = Timer.GetDeltaTime();

		Window.BeginFrame();
		CKeyboard::Update();
		CRenderer::BeginFrame();

		CPhysicsWorld::Update(DeltaTime);

		Camera->SetViewportSize(WindowData.Width, WindowData.Height);
		CRenderer::BeginScene(*Camera);

		Player->Tick(DeltaTime);
		Platform->Tick(DeltaTime);

		UI_MenuBar();
		ImGui::Text("Delta Time: %.3fms", DeltaTime);

		DrawPlayer(*Player);
		DrawPlatform(*Platform);

		ImGui::Begin("Physics");
		const b2Vec2 G = b2World_GetGravity(CPhysicsWorld::GetID());
		ImGui::Text("Gravity: (%.1f, %.1f)", G.x, G.y);
		glm::vec2 PlayerBodyPos = Player->GetBody().GetPosition();
		ImGui::Text("Player Body: (%.2f, %.2f)", PlayerBodyPos.x, PlayerBodyPos.y);
		PlayerBodyPos = Player->GetPosition();
		ImGui::Text("Player TC: (%.2f, %.2f)", PlayerBodyPos.x, PlayerBodyPos.y);
		ImGui::End();

		CRenderer::Flush();
		CRenderer::EndFrame();
		CKeyboard::TransitionPressedKeys();
		Window.EndFrame();
	}

	Window.Destroy();

	return 0;
}

std::unique_ptr<CPlayer> CreatePlayer()
{
	FCapsule PlayerCapsule;
	PlayerCapsule.P0 = { 0.0f, 0.0f };
	PlayerCapsule.P1 = { 0.0f, 0.20f };
	PlayerCapsule.Radius = 0.10f;
	const float CapsuleDist = glm::distance(PlayerCapsule.P0, PlayerCapsule.P1);
	LK_DEBUG("Player Capsule Dist: {}", CapsuleDist);
	PlayerSize = { CapsuleDist, CapsuleDist };

	FBodySpecification BodySpec;
	BodySpec.Type = EBodyType::Dynamic;
	BodySpec.Position = { 0.0f, 1.0f };
	BodySpec.Friction = 0.10f;
	BodySpec.Density = 0.60f;
	BodySpec.LinearDamping = 0.50f;
	BodySpec.MotionLock = EMotionLock_Z;
	BodySpec.Shape.emplace<FCapsule>(PlayerCapsule);
	std::unique_ptr<CPlayer> Player = std::make_unique<CPlayer>(BodySpec);

	const CBody& PlayerBody = Player->GetBody();
	const FPlayerData& PlayerData = Player->GetData();
	FTransformComponent& PlayerTC = Player->GetTransformComponent();
	glm::vec3& PlayerPos = PlayerTC.Translation;
	glm::vec3& PlayerScale = PlayerTC.Scale;
	b2World_SetPreSolveCallback(CPhysicsWorld::GetID(), PreSolveStatic, Player.get());

	Player->OnJumped.Add([](const FPlayerData& PlayerData)
	{
		LK_TRACE("Player {} jumped", PlayerData.ID);
	});

	Player->OnLanded.Add([](const FPlayerData& PlayerData)
	{
		LK_TRACE("Player {} landed", PlayerData.ID);
	});

	return Player;
}

void DrawPlayer(const CPlayer& Player)
{
	const FCapsule* Capsule = Player.GetBody().TryGetShape<EShape::Capsule>();
	if (Capsule)
	{
		CRenderer::DrawQuad(Player.GetPosition(), PlayerSize, *PlayerTexture, FColor::White);
	}
}

std::unique_ptr<CActor> CreatePlatform()
{
	FBodySpecification PlatformSpec;
	PlatformSpec.Position = { 0.0f, -0.80 };
	PlatformSpec.Type = EBodyType::Static;
	PlatformSpec.Flags = EBodyFlag_PreSolveEvents;

	FPolygon PlatformPolygon = {
		.Size = { 2.0f, 0.08f }
	};
	PlatformSpec.Shape.emplace<FPolygon>(PlatformPolygon);
	std::unique_ptr<CActor> Platform = std::make_unique<CActor>(PlatformSpec);
	FTransformComponent& PlatformTC = Platform->GetTransformComponent();
	PlatformTC.SetScale(PlatformPolygon.Size);

	return Platform;
}

void DrawPlatform(const CActor& Platform)
{
	const FTransformComponent& TC = Platform.GetTransformComponent();
	CRenderer::DrawQuad(Platform.GetPosition(), TC.Scale, *PlatformTexture, FColor::White);
}

bool PreSolve(b2ShapeId ShapeA, b2ShapeId ShapeB, b2Vec2 Point, b2Vec2 Normal, const CPlayer& Player)
{
	LK_ASSERT(b2Shape_IsValid(ShapeA));
	LK_ASSERT(b2Shape_IsValid(ShapeB));

	const b2ShapeId PlayerShapeID = Player.GetBody().GetShapeID();

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

