#include "testlevel.h"

#include "core/window.h"
#include "core/timer.h"
#include "core/selectioncontext.h"
#include "core/input/keyboard.h"
#include "core/input/mouse.h"
#include "core/math/math.h"
#include "game/player.h"
#include "renderer/renderer.h"
#include "renderer/debugrenderer.h"
#include "renderer/ui/ui.h"
#include "renderer/ui/widgets.h"
#include "physics/physicsworld.h"
#include "physics/body.h"

#define PLAYER_SHAPE_CAPSULE 0

namespace platformer2d::Level {

	namespace
	{
		/*************************************
		 *        GAME SPECIFICATION
		/*************************************/
		FGameSpecification GameSpec = {
			.Name = "TestLevel",
			.Gravity = { 0.0f, -5.0f },
			.Zoom = 0.32f,
			.PlayerBody = {
				.Type = EBodyType::Dynamic,
#if PLAYER_SHAPE_CAPSULE
				.Shape = FCapsule{
					.P0 = { 0.0f, 0.0f },
					.P1 = { 0.0f, 0.15f },
					.Radius = 0.10f,
				},
#else
				.Shape = FPolygon{
					.Size = { 0.20f, 0.24f },
					.Radius = 0.12f,
					.Rotation = glm::radians(0.0f),
				},
#endif
				.Position = { 0.0f, 0.50f },
				.Friction = 0.10f,
				.Density = 0.60f,
				.LinearDamping = 0.50f,
				.MotionLock = EMotionLock_Z,
			},
		};

		constexpr float UI_BG_ALPHA = 0.70f;
		constexpr const char* UI_ID_LEVEL = "Level";
		constexpr const char* UI_ID_PLAYER = "Player";

		std::vector<std::shared_ptr<CActor>> Actors;
		std::weak_ptr<CActor> RotatingPlatform;
	}

	bool PreSolve(b2ShapeId ShapeA, b2ShapeId ShapeB, b2Vec2 Point, b2Vec2 Normal, void* Ctx);

	CTestLevel::CTestLevel()
		: IGameInstance(GameSpec)
	{
		Instance = this;
		Actors.clear();
	}

	void CTestLevel::Initialize()
	{
		LK_VERIFY(Instance);
		LK_DEBUG_TAG("TestLevel", "Initialize");
		LK_ASSERT(Player == nullptr);

		CActor::OnActorCreated.Add([](const FActorHandle Handle, std::weak_ptr<CActor> ActorRef)
		{
			if (std::shared_ptr<CActor> Actor = ActorRef.lock(); Actor != nullptr)
			{
				LK_DEBUG_TAG("TestLevel", "Actor created: {} ({})", Actor->GetName(), Handle);
				Actors.emplace_back(Actor);
			}
		});

		const FGameSpecification& Spec = GetSpecification();
		CPhysicsWorld::SetGravity(Spec.Gravity);

		CreatePlayer();
		CreatePlatform();
		LK_VERIFY(Player && Platform);

		CreateTerrain();

		CCamera* Camera = GetActiveCamera();
		LK_VERIFY(Camera);
		Camera->SetZoom(Spec.Zoom);

		UI::OnGameMenuOpened.Add([](const bool Opened)
		{
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
		LK_TRACE_TAG("TestLevel", "Destroy");
		Player.reset();

		LK_DEBUG_TAG("TestLevel", "Releasing level resources");
		Actors.clear();
	}

	void CTestLevel::OnAttach()
	{
		LK_TRACE_TAG("TestLevel", "OnAttach");
		Initialize();
	}

	void CTestLevel::OnDetach()
	{
		LK_TRACE_TAG("TestLevel", "OnDetach");
		Destroy();
	}

	void CTestLevel::Tick(const float DeltaTime)
	{
		CCamera& Camera = Player->GetCamera();
		Camera.SetViewportSize(ViewportWidth, ViewportHeight);
		CRenderer::BeginScene(Camera);

		Player->Tick(DeltaTime);
		Platform->Tick(DeltaTime);

		Tick_Objects();

		/* Render player. */
#if PLAYER_SHAPE_CAPSULE
		const FCapsule* Capsule = Player->GetBody().TryGetShape<EShape::Capsule>();
		if (Capsule)
		{
			glm::vec2 PlayerSize = Player->GetBody().GetSize();
			const glm::vec3 Scale = Player->GetTransformComponent().GetScale();
			PlayerSize *= glm::vec2(Scale.x, Scale.y);
			CRenderer::DrawQuad(
				Player->GetPosition(),
				PlayerSize,
				CRenderer::GetTexture(Player->GetTexture()),
				FColor::White,
				glm::degrees(Player->GetRotation())
			);
		}
#else
		const FPolygon* Polygon = Player->GetBody().TryGetShape<EShape::Polygon>();
		if (Polygon)
		{
			const glm::vec2 PlayerSize = Player->GetSize();
			CRenderer::DrawQuad(
				Player->GetPosition(),
				PlayerSize,
				CRenderer::GetTexture(Player->GetTexture()),
				FColor::White,
				glm::degrees(Player->GetRotation())
			);
		}
#endif

		/* Render level. */
		for (const std::shared_ptr<CActor>& Actor : Actors)
		{
			const FTransformComponent& TC = Actor->GetTransformComponent();
			CRenderer::DrawQuad(
				Actor->GetPosition(),
				TC.Scale,
				Actor->GetTexture(),
				Actor->GetColor(),
				glm::degrees(TC.GetRotation2D())
			);
		}

		/* Draw dark overlay whenever the pause menu is open. */
		if (UI::IsGameMenuOpen())
		{
			static constexpr glm::vec4 OverlayColor = { 0.10f, 0.10f, 0.10f, 0.80f };
			CRenderer::DrawQuad({ 0.0f, 0.0f }, { ViewportWidth, ViewportHeight }, OverlayColor);
		}
	}

	CCamera* CTestLevel::GetActiveCamera() const
	{
		return (Player ? &Player->GetCamera() : nullptr);
	}

	void CTestLevel::RenderUI()
	{
		UI_Level();
		UI_Player();
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
		Spec.Name = "Metal";
		Spec.Position = { 0.0f, -0.72f };
		Spec.Type = EBodyType::Static;
		Spec.Flags = EBodyFlag_PreSolveEvents;

		FPolygon Polygon = {
			.Size = { 2.0f, 0.08f }
		};
		Spec.Shape.emplace<FPolygon>(Polygon);

		Platform = CActor::Create<CActor>(Spec, ETexture::Metal);
		FTransformComponent& TC = Platform->GetTransformComponent();
		TC.SetScale(Polygon.Size);
	}

	void CTestLevel::CreateTerrain()
	{
		/* Object 1. */
		{
			FBodySpecification Spec;
			Spec.Type = EBodyType::Static;
			Spec.Position = { 2.38f, -0.18f };
			Spec.Flags = EBodyFlag_PreSolveEvents;
			Spec.Name = "Spawn-Right-Platform";

			FPolygon Polygon = {
				.Size = { 3.0f, 0.09 },
				.Rotation = glm::radians(21.81f),
			};
			Spec.Shape.emplace<FPolygon>(Polygon);

			std::shared_ptr<CActor> Actor = CActor::Create<CActor>(Spec, ETexture::Metal);
			Actor->SetColor(FColor::LightGreen);
		}

		/* Object 2. */
		{
			FBodySpecification Spec;
			Spec.Type = EBodyType::Static;
			Spec.Position = { -1.48f, -0.04f };
			Spec.Flags = EBodyFlag_PreSolveEvents;
			Spec.Name = "Spawn-Wall-Left";

			FPolygon Polygon = {
				.Size = { 1.45f, 0.95f },
				.Rotation = glm::radians(90.0f),
			};
			Spec.Shape.emplace<FPolygon>(Polygon);

			std::shared_ptr<CActor> Actor = CActor::Create<CActor>(Spec, ETexture::Bricks);
		}

		/* Object 3. */
		{
			FBodySpecification Spec;
			Spec.Type = EBodyType::Static;
			Spec.Position = { -0.43f, -0.10f };
			Spec.Flags = EBodyFlag_PreSolveEvents;
			Spec.Name = "FlyingPlatform-1";

			FPolygon Polygon = {
				.Size = { 0.40f, 0.06f },
				.Rotation = glm::radians(0.0f),
			};
			Spec.Shape.emplace<FPolygon>(Polygon);

			std::shared_ptr<CActor> Actor = CActor::Create<CActor>(Spec);
			Actor->SetColor(FColor::Convert(RGBA32::Magenta));
		}

		/* Object 4. */
		{
			FBodySpecification Spec;
			Spec.Type = EBodyType::Static;
			Spec.Position = { 0.43f, 0.02f };
			Spec.Flags = EBodyFlag_PreSolveEvents;
			Spec.Name = "Rotating-Platform";

			FPolygon Polygon = {
				.Size = { 0.40f, 0.06f },
				.Rotation = glm::radians(0.0f),
			};
			Spec.Shape.emplace<FPolygon>(Polygon);

			std::shared_ptr<CActor> Actor = CActor::Create<CActor>(Spec);
			Actor->SetColor(FColor::Convert(RGBA32::DarkCyan));
			RotatingPlatform = Actor;
		}
	}

	void CTestLevel::Tick_Objects()
	{
		if (std::shared_ptr<CActor> Actor = RotatingPlatform.lock(); Actor != nullptr)
		{
			Actor->SetRotation(Actor->GetRotation() + glm::radians(0.75f));
		}
	}

	void CTestLevel::UI_Level()
	{
		ImGui::SetNextWindowBgAlpha(UI_BG_ALPHA);
		if (!ImGui::Begin(UI_ID_LEVEL))
		{
			ImGui::End();
			return;
		}

		UI::Font::Push(EFont::SourceSansPro, EFontSize::Regular, EFontModifier::Normal);

		const b2Vec2 G = b2World_GetGravity(CPhysicsWorld::GetID());
		ImGui::Text("Gravity: (%.1f, %.1f)", G.x, G.y);

		ImGui::Text("Actors: %d", Actors.size());

		UI::Draw::ActorNode(*Player);
		for (auto& Actor : Actors)
		{
			UI::Draw::ActorNode(*Actor);
		}

		ImGui::Dummy(ImVec2(0, 16));
		UI_TextureModifier();

		UI::Font::Pop();
		ImGui::End();
	}

	void CTestLevel::UI_Player()
	{
		const CBody& PlayerBody = Player->GetBody();
		const FPlayerData& PlayerData = Player->GetData();

		ImGui::SetNextWindowBgAlpha(UI_BG_ALPHA);
		if (!ImGui::Begin("Player"))
		{
			ImGui::End();
			return;
		}

		glm::vec2 PlayerBodyPos = Player->GetBody().GetPosition();
		ImGui::Text("Position: (%.2f, %.2f)", PlayerBodyPos.x, PlayerBodyPos.y);
		PlayerBodyPos = Player->GetPosition();
		ImGui::Text("Transform Component: (%.2f, %.2f)", PlayerBodyPos.x, PlayerBodyPos.y);
		ImGui::Text("Jump State: %s", PlayerData.bJumping ? "Jumping" : "On ground");

		ImGui::Text("Camera Zoom: %.2f", Player->GetCamera().GetZoom());

		const glm::vec2 LinearVelocity = PlayerBody.GetLinearVelocity();
		ImGui::Text("Linear Velocity: (%.2f, %.2f)", LinearVelocity.x, LinearVelocity.y);

		ImGui::PushItemWidth(160.0f);
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

		ImGui::Dummy(ImVec2(0, 6));
		UI::Draw::ActorNode_VectorControl(*Player);
		ImGui::Dummy(ImVec2(0, 6));

		if (ImGui::Button("Rotate Platform"))
		{
			Platform->SetRotation(Platform->GetRotation() + glm::radians(45.0f));
		}

		ImGui::End();
	}

	void CTestLevel::UI_TextureModifier()
	{
		static const std::array<const char*, CRenderer::MAX_TEXTURES> TextureNames = {
			Enum::ToString(ETexture::White),
			Enum::ToString(ETexture::Player),
			Enum::ToString(ETexture::Metal),
			Enum::ToString(ETexture::Bricks),
			Enum::ToString(ETexture::Wood),
		};

		static constexpr float ButtonPaddingY = 7.0f;
		static constexpr ImVec2 ButtonSize(84, 42);
		static constexpr float ItemWidth = 2.0f * ButtonSize.x;

		static int SelectedTextureIdx = 0;
		static const char* SelectedTexture = TextureNames[SelectedTextureIdx];

		ImGui::Separator();
		ImGui::Dummy(ImVec2(0, 14));
		const ImVec2 Avail = ImGui::GetContentRegionAvail();

		static const char* ComboName = "Texture";
		const ImVec2 ComboNameLen = ImGui::CalcTextSize(ComboName);

		UI::FScopedStyle FramePadding(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
		UI::FScopedStyle FrameRounding(ImGuiStyleVar_FrameRounding, 8.0f);

		UI::ShiftCursorX(20.0f);
		ImGui::Text(ComboName);

		ImGui::SameLine((Avail.x * 0.50f) - (ItemWidth * 0.50f) + ButtonPaddingY);
		UI::ShiftCursorY(-4.0f);

		ImGui::SetNextItemWidth(ItemWidth);
		if (ImGui::BeginCombo("##Texture", TextureNames[SelectedTextureIdx]))
		{
			SelectedTexture = TextureNames[SelectedTextureIdx];
			for (int Idx = 0; Idx < TextureNames.size(); Idx++)
			{
				const char* Option = TextureNames[Idx];
				if (Option == nullptr)
				{
					continue;
				}

				const bool IsSelected = (Option == SelectedTexture);
				if (ImGui::Selectable(Option, IsSelected))
				{
					SelectedTextureIdx = Idx;
				}
			}

			ImGui::EndCombo();
		}

		const ETexture Texture = static_cast<ETexture>(SelectedTextureIdx);
		const CTexture& TextureRef = CRenderer::GetTexture(Texture);

		/* Texture preview. */
		ImGui::SameLine(0.0f, 12.0f);
		UI::ShiftCursorY(-4.0f);
		ImGui::Image(
			static_cast<ImU64>(TextureRef.GetID()),
			ImVec2(32.0f, 32.0f),
			ImVec2(0.0f, 1.0f), /* Uv0. */
			ImVec2(1.0f, 0.0f)  /* Uv1. */
		);

		static constexpr std::string_view Marker = "assets/textures/";
		auto StripPrefix = [](const std::filesystem::path& Path) -> std::string
		{
			const std::string Str = Path.generic_string();
			const std::size_t Pos = Str.find(Marker);
			if (Pos != std::string::npos)
			{
				return Str.substr(Pos);
			}
			return Str;
		};

		ImGui::Dummy(ImVec2(0, 6));
		{
			UI::FScopedFont Font(UI::Font::Get(EFont::SourceSansPro, EFontSize::Regular, EFontModifier::BoldItalic));
			ImGui::Indent();
			ImGui::Text("Size:%-4s%dx%d", " ", TextureRef.GetWidth(), TextureRef.GetHeight());
			const std::string TrimmedPath = StripPrefix(TextureRef.GetFilePath());
			ImGui::Text("Path:%-4s%s", " ", TrimmedPath.c_str());
			ImGui::Unindent();
		}
		ImGui::Dummy(ImVec2(0, 12));

		{
			UI::FScopedStyle FrameRounding(ImGuiStyleVar_FrameRounding, 10);
			UI::FScopedStyle FramePadding(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
			UI::FScopedStyle FrameBorder(ImGuiStyleVar_FrameBorderSize, 2.0f);
			UI::FScopedColor ButtonCol(ImGuiCol_Button, RGBA32::Titlebar::Default);
			UI::FScopedColor ButtonActiveCol(ImGuiCol_ButtonActive, RGBA32::LightGray);
			UI::FScopedColor ButtonHoveredCol(ImGuiCol_ButtonHovered, RGBA32::SelectionMuted);

			UI::ShiftCursorX(60.0f);
			ImGui::Text("Wrap");

			ImGui::SameLine((Avail.x * 0.50f) - (ItemWidth * 0.50f) + ButtonPaddingY);
			UI::ShiftCursorY(-ButtonPaddingY);
			if (ImGui::Button("Clamp", ButtonSize))
			{
				CRenderer::GetTexture(Texture).SetWrap(ETextureWrap::Clamp);
			}
			ImGui::SameLine(0.0f, ButtonPaddingY);
			UI::ShiftCursorY(-ButtonPaddingY);
			if (ImGui::Button("Repeat", ButtonSize))
			{
				CRenderer::GetTexture(Texture).SetWrap(ETextureWrap::Repeat);
			}

			ImGui::Dummy(ImVec2(0, 6));

			UI::ShiftCursorX(60.0f);
			ImGui::Text("Filter");

			ImGui::SameLine((Avail.x * 0.50f) - (ItemWidth * 0.50f) + ButtonPaddingY);
			UI::ShiftCursorY(-ButtonPaddingY);
			if (ImGui::Button("Linear", ButtonSize))
			{
				CRenderer::GetTexture(Texture).SetFilter(ETextureFilter::Linear);
			}

			ImGui::SameLine(0.0f, ButtonPaddingY);
			UI::ShiftCursorY(-ButtonPaddingY);
			if (ImGui::Button("Nearest", ButtonSize))
			{
				CRenderer::GetTexture(Texture).SetFilter(ETextureFilter::Nearest);
			}
		}
	}

	bool PreSolve(b2ShapeId ShapeA, b2ShapeId ShapeB, b2Vec2 Point, b2Vec2 Normal, void* Ctx)
	{
		LK_ASSERT(b2Shape_IsValid(ShapeA));
		LK_ASSERT(b2Shape_IsValid(ShapeB));
		CPlayer& Player = *static_cast<CPlayer*>(Ctx);
		const b2ShapeId PlayerShapeID = Player.GetBody().GetShapeID();

		const bool InvolvesPlayer = B2_ID_EQUALS(ShapeA, PlayerShapeID) || B2_ID_EQUALS(ShapeB, PlayerShapeID);
		if (!InvolvesPlayer)
		{
			return true; /* Enable normal contacts. */
		}

		/* Make normal point from platform to player. */
		if (B2_ID_EQUALS(ShapeA, PlayerShapeID))
		{
			Normal.x = -Normal.x;
			Normal.y = -Normal.y;
		}

		const b2Vec2 Up = { 0.0f, 1.0f };
		const float UpDot = Normal.x * Up.x + Normal.y * Up.y;
		if (UpDot <= 0.0f)
		{
			/* Side/ceiling/backface -> behave as a solid. */
			return true;
		}

		const b2BodyId PlayerBody = Player.GetBody().GetID();
		const b2Vec2 V = b2Body_GetLinearVelocity(PlayerBody);
		const float Vn = V.x * Normal.x + V.y * Normal.y;
		if (Vn > 0.0f)
		{
			/* Moving along the normal (from below toward the platform) -> ignore contact. */
			return false;
		}

		return true;
	}

}
