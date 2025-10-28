#include "testlevel.h"

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

namespace platformer2d::Level {

	namespace
	{
		/*************************************
		 *        GAME SPECIFICATION
		/*************************************/
		FGameSpecification GameSpec = {
			.Name = "TestLevel",
			.Gravity = { 0.0f, -5.0f },
			.PlayerBody = {
				.Type = EBodyType::Dynamic,
				.Shape = FCapsule{
					.P0 = { 0.0f, 0.0f },
					.P1 = { 0.0f, 0.15f },
					.Radius = 0.10f,
				},
				.Position = { 0.0f, 1.0f },
				.Friction = 0.10f,
				.Density = 0.60f,
				.LinearDamping = 0.50f,
				.MotionLock = EMotionLock_Z,
			},
		};

		std::shared_ptr<CActor> Platform = nullptr;

		std::shared_ptr<CTexture> PlayerTexture = nullptr;
		std::shared_ptr<CTexture> PlatformTexture = nullptr;

		std::vector<std::weak_ptr<CActor>> Actors;
	}

	std::shared_ptr<CActor> CreatePlatform();
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

		Player = std::make_unique<CPlayer>(Spec.PlayerBody);
		LK_VERIFY(Player);
		b2World_SetPreSolveCallback(CPhysicsWorld::GetWorldID(), PreSolve, Player.get());

		Platform = CreatePlatform();

		const std::unordered_map<ETexture, std::shared_ptr<CTexture>>& Textures = CRenderer::GetTextures();
		PlayerTexture = Textures.at(ETexture::Player);
		PlatformTexture = Textures.at(ETexture::Platform);
		LK_VERIFY(PlayerTexture && PlatformTexture);

		Player->OnJumped.Add([](const FPlayerData& PlayerData)
		{
			LK_TRACE("Player {} jumped", PlayerData.ID);
		});

		Player->OnLanded.Add([](const FPlayerData& PlayerData)
		{
			LK_TRACE("Player {} landed", PlayerData.ID);
		});

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
	}

	void CTestLevel::Destroy()
	{
		LK_DEBUG_TAG("TestLevel", "Destroy");
		Player.reset();
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
			CRenderer::DrawQuad(Player->GetPosition(), PlayerSize, *PlayerTexture, FColor::White);
		}

		const FTransformComponent& TC = Platform->GetTransformComponent();
		CRenderer::DrawQuad(Platform->GetPosition(), TC.Scale, *PlatformTexture, FColor::White);

		for (const auto& ActorRef : Actors)
		{
			if (std::shared_ptr<CActor> Actor = ActorRef.lock(); Actor != nullptr)
			{
				const FTransformComponent& TC = Actor->GetTransformComponent();
				CRenderer::DrawQuad(Actor->GetPosition(), TC.Scale, *CRenderer::GetWhiteTexture(), FColor::White);
			}
		}
	}

	void CTestLevel::RenderUI()
	{
		UI_Player();
		UI_Physics();
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
		const b2Vec2 G = b2World_GetGravity(CPhysicsWorld::GetWorldID());
		ImGui::Text("Gravity: (%.1f, %.1f)", G.x, G.y);
		ImGui::End();
	}

	std::shared_ptr<CActor> CreatePlatform()
	{
		FBodySpecification PlatformSpec;
		PlatformSpec.Position = { 0.0f, -0.80 };
		PlatformSpec.Type = EBodyType::Static;
		PlatformSpec.Flags = EBodyFlag_PreSolveEvents;

		FPolygon PlatformPolygon = {
			.Size = { 2.0f, 0.08f }
		};
		PlatformSpec.Shape.emplace<FPolygon>(PlatformPolygon);

		std::shared_ptr<CActor> Platform = CActor::Create<CActor>(PlatformSpec);
		FTransformComponent& PlatformTC = Platform->GetTransformComponent();
		PlatformTC.SetScale(PlatformPolygon.Size);

		return Platform;
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
