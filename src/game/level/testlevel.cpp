#include "testlevel.h"

#include "core/window.h"
#include "core/timer.h"
#include "core/input/keyboard.h"
#include "core/input/mouse.h"
#include "core/math/math.h"
#include "game/player.h"
#include "renderer/renderer.h"
#include "renderer/debugrenderer.h"
#include "renderer/ui.h"
#include "physics/physicsworld.h"
#include "physics/body.h"

namespace platformer2d::Level {

	namespace
	{
		/*************************************
		 *        GAME SPECIFICATION
		/*************************************/
		FGameSpecification GameSpec = {
			.Name = "TestLevel",
			.Gravity = { 0.0f, -5.0f },
			.Zoom = 0.25f,
			.PlayerBody = {
				.Type = EBodyType::Dynamic,
				.Shape = FCapsule{
					.P0 = { 0.0f, 0.0f },
					.P1 = { 0.0f, 0.20f },
					.Radius = 0.10f,
				},
				.Position = { 0.0f, 1.0f },
				.Friction = 0.10f,
				.Density = 0.60f,
				.LinearDamping = 0.50f,
				.MotionLock = EMotionLock_Z,
			},
		};

		std::vector<std::weak_ptr<CActor>> Actors;
	}

	bool PreSolve(b2ShapeId ShapeA, b2ShapeId ShapeB, b2Vec2 Point, b2Vec2 Normal, void* Ctx);

	CTestLevel::CTestLevel()
		: IGameInstance(GameSpec)
	{
		Actors.clear();
	}

	CTestLevel::~CTestLevel()
	{
	}

	void CTestLevel::Initialize()
	{
		LK_DEBUG_TAG("TestLevel", "Initialize");
		LK_ASSERT(Player == nullptr);

		CActor::OnActorCreated.Add([](const FActorHandle Handle, std::weak_ptr<CActor> Actor)
		{
			LK_INFO_TAG("TestLevel", "Created actor: {}", Handle);
			Actors.emplace_back(Actor);
		});

		const FGameSpecification& Spec = GetSpecification();
		CPhysicsWorld::SetGravity(Spec.Gravity);

		CreatePlayer();
		CreatePlatform();
		LK_VERIFY(Player && Platform);

		/* Remove player and platform from actors vector to not draw multiple times. */
		const FActorHandle PlayerHandle = Player->GetHandle();
		const FActorHandle PlatformHandle = Platform->GetHandle();
		auto IsPlayerOrPlatform = [PlayerHandle, PlatformHandle](std::weak_ptr<CActor> ActorRef) -> bool
		{
			if (std::shared_ptr<CActor> Actor = ActorRef.lock(); Actor != nullptr)
			{
				const FActorHandle Handle = Actor->GetHandle();
				return ((Handle == PlayerHandle) || (Handle == PlatformHandle));
			}
			return false;
		};
		std::erase_if(Actors, IsPlayerOrPlatform);

		CreateGround();

		if (CCamera* Camera = GetActiveCamera(); Camera != nullptr)
		{
			Camera->SetZoom(Spec.Zoom);
		}

		UI::OnGameMenuOpened.Add([](const bool Opened)
		{
			LK_DEBUG_TAG("TestLevel", "Game Menu: {}", Opened ? "Opened" : "Closed");
			if (Opened)
			{
				CPhysicsWorld::Pause();
			}
			else
			{
				CPhysicsWorld::Unpause();
			}
		});
	}

	void CTestLevel::Destroy()
	{
		LK_DEBUG_TAG("TestLevel", "Destroy");
		Player.reset();
	}

	void CTestLevel::OnAttach()
	{
		LK_DEBUG_TAG("TestLevel", "OnAttach");
		Initialize();
	}

	void CTestLevel::OnDetach()
	{
		LK_DEBUG_TAG("TestLevel", "OnDetach");
		Destroy();
	}

	void CTestLevel::Tick(const float DeltaTime)
	{
		CCamera& Camera = Player->GetCamera();
		CRenderer::BeginScene(Camera);
		Camera.SetViewportSize(ViewportWidth, ViewportHeight);

		Player->Tick(DeltaTime);
		Platform->Tick(DeltaTime);

		const FCapsule* Capsule = Player->GetBody().TryGetShape<EShape::Capsule>();
		if (Capsule)
		{
			const glm::vec3 Scale = Player->GetTransformComponent().GetScale();
			const float Dist = glm::distance(Capsule->P0, Capsule->P1);
			glm::vec2 PlayerSize = { Dist, Dist };
			PlayerSize *= glm::vec2(Scale.x, Scale.y);
			CRenderer::DrawQuad(Player->GetPosition(), PlayerSize, CRenderer::GetTexture(Player->GetTexture()), FColor::White);
		}

		const FTransformComponent& TC = Platform->GetTransformComponent();
		CRenderer::DrawQuad(Platform->GetPosition(), TC.Scale, CRenderer::GetTexture(Platform->GetTexture()), FColor::White);

		for (const auto& ActorRef : Actors)
		{
			if (std::shared_ptr<CActor> Actor = ActorRef.lock(); Actor != nullptr)
			{
				const FTransformComponent& TC = Actor->GetTransformComponent();
				CRenderer::DrawQuad(Actor->GetPosition(), TC.Scale, Actor->GetTexture(), Actor->GetColor());
			}
		}

		/* Draw dark overlay whenever the pause menu is open. */
		if (UI::IsGameMenuOpen())
		{
			const glm::vec4 OverlayColor = { 0.10f, 0.10f, 0.10f, 0.80f };
			CRenderer::DrawQuad({ 0.0f, 0.0f }, { ViewportWidth, ViewportHeight }, OverlayColor);
		}
	}

	CCamera* CTestLevel::GetActiveCamera() const
	{
		if (Player)
		{
			return &Player->GetCamera();
		}

		return nullptr;
	}

	void CTestLevel::RenderUI()
	{
		UI_Player();
		UI_Physics();
	}

	void CTestLevel::CreatePlayer()
	{
		const FGameSpecification& Spec = GetSpecification();
		Player = std::make_unique<CPlayer>(Spec.PlayerBody, ETexture::Player);
		b2World_SetPreSolveCallback(CPhysicsWorld::GetID(), PreSolve, Player.get());

		Player->OnJumped.Add([](const FPlayerData& PlayerData)
		{
			LK_TRACE("Player {} jumped", PlayerData.ID);
		});

		Player->OnLanded.Add([](const FPlayerData& PlayerData)
		{
			LK_TRACE("Player {} landed", PlayerData.ID);
		});
	}

	void CTestLevel::CreatePlatform()
	{
		FBodySpecification Spec;
		Spec.Position = { 0.0f, -0.80 };
		Spec.Type = EBodyType::Static;
		Spec.Flags = EBodyFlag_PreSolveEvents;

		FPolygon Polygon = {
			.Size = { 2.0f, 0.08f }
		};
		Spec.Shape.emplace<FPolygon>(Polygon);

		Platform = CActor::Create<CActor>(Spec, ETexture::Platform);
		FTransformComponent& TC = Platform->GetTransformComponent();
		TC.SetScale(Polygon.Size);
	}

	void CTestLevel::CreateGround()
	{
		FBodySpecification Spec;
		Spec.Position = { 0.0f, -0.90 };
		Spec.Type = EBodyType::Static;
		Spec.Flags = EBodyFlag_PreSolveEvents;

		FPolygon Polygon = {
			.Size = { 10.0f, 0.12f }
		};
		Spec.Shape.emplace<FPolygon>(Polygon);

		Ground = CActor::Create<CActor>(Spec);
		Ground->SetColor(FColor::Red);

		FTransformComponent& TC = Ground->GetTransformComponent();
		TC.SetScale(Polygon.Size);
	}

	void CTestLevel::UI_Player()
	{
		const CBody& PlayerBody = Player->GetBody();
		const FPlayerData& PlayerData = Player->GetData();

		ImGui::Begin("Player");
		glm::vec2 PlayerBodyPos = Player->GetBody().GetPosition();
		ImGui::Text("Body: (%.2f, %.2f)", PlayerBodyPos.x, PlayerBodyPos.y);
		PlayerBodyPos = Player->GetPosition();
		ImGui::Text("Transform Component: (%.2f, %.2f)", PlayerBodyPos.x, PlayerBodyPos.y);
		ImGui::Text("Jump State: %s", PlayerData.bJumping ? "Jumping" : "On ground");

		ImGui::Dummy(ImVec2(0, 8));

		ImGui::PushItemWidth(220.0f);
		const glm::vec2 PlayerLinearVelocity = PlayerBody.GetLinearVelocity();
		ImGui::Text("Body Linear Velocity: (%.2f, %.2f)", PlayerLinearVelocity.x, PlayerLinearVelocity.y);

		float PlayerJumpImpulse = Player->GetJumpImpulse();
		ImGui::SliderFloat("Jump Impulse", &PlayerJumpImpulse, 0.0f, 10.0f, "%.5f");
		if (ImGui::IsItemActive()) Player->SetJumpImpulse(PlayerJumpImpulse);

		float PlayerDirForce = Player->GetDirectionForce();
		ImGui::SliderFloat("Direction Force", &PlayerDirForce, 0.0f, 10.0f, "%.5f");
		if (ImGui::IsItemActive()) Player->SetDirectionForce(PlayerDirForce);

		static float BodyScale = 1.0f;
		ImGui::SliderFloat("Body Scale", &BodyScale, 0.0f, 2.0f, "%.2f");
		ImGui::SameLine();
		if (ImGui::Button("Apply##Scale")) PlayerBody.SetScale(BodyScale);

		float Mass = PlayerBody.GetMass();
		ImGui::SliderFloat("Mass", &Mass, 0.0f, 10.0f, "%.2f");
		if (ImGui::IsItemActive()) PlayerBody.SetMass(Mass);

		float PlayerFriction = PlayerBody.GetFriction();
		ImGui::SliderFloat("Friction", &PlayerFriction, 0.0f, 2.0f, "%.3f");
		if (ImGui::IsItemActive()) PlayerBody.SetFriction(PlayerFriction);

		float PlayerRestitution = PlayerBody.GetRestitution();
		ImGui::SliderFloat("Restitution", &PlayerRestitution, 0.0f, 2.0f, "%.3f");
		if (ImGui::IsItemActive()) PlayerBody.SetRestitution(PlayerRestitution);

		ImGui::PopItemWidth();
		ImGui::End();
	}

	void CTestLevel::UI_Physics()
	{
		ImGui::Begin("Physics");
		const b2Vec2 G = b2World_GetGravity(CPhysicsWorld::GetID());
		ImGui::Text("Gravity: (%.1f, %.1f)", G.x, G.y);
		ImGui::End();
	}

	bool PreSolve(b2ShapeId ShapeA, b2ShapeId ShapeB, b2Vec2 Point, b2Vec2 Normal, void* Ctx)
	{
		LK_ASSERT(b2Shape_IsValid(ShapeA));
		LK_ASSERT(b2Shape_IsValid(ShapeB));

		CPlayer& Player = *static_cast<CPlayer*>(Ctx);
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


}
