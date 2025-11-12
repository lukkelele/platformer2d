#include "testlevel.h"

#include <fstream>
#include <istream>
#include <numeric>

#include "core/window.h"
#include "core/timer.h"
#include "core/selectioncontext.h"
#include "core/input/keyboard.h"
#include "core/input/mouse.h"
#include "core/math/math.h"
#include "game/player.h"
#include "game/spawner.h"
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
				.Friction = 0.750f,
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

		struct FCloud
		{
			glm::vec3 Position = { 0.0f, 0.0f, 0.60f };
			glm::vec2 Size{};
		};
		std::vector<FCloud> Clouds = {
			FCloud({  1.49, 2.42, 0.0001 }, { 0.93, 0.74 }),
			FCloud({ -0.70, 2.20, 0.0002 }, { 1.15, 0.92 }),
			FCloud({  1.50, 1.15, 0.0003 }, { 1.05, 0.84 }),
			FCloud({  2.80, 0.39, 0.0004 }, { 0.95, 0.76 }),
			FCloud({ -3.15, 2.30, 0.0005 }, { 1.13, 0.90 }),
			FCloud({ -2.68, 0.58, 0.0006 }, { 0.99, 0.80 }),
			FCloud({ -1.75, 1.34, 0.0007 }, { 1.03, 0.90 }),
			FCloud({ -0.58, 1.63, 0.0008 }, { 0.99, 0.80 }),
			FCloud({  0.84, 0.38, 0.0009 }, { 0.99, 0.80 }),
			FCloud({ -0.20, 0.80, 0.0010 }, { 0.99, 0.80 }),
		};
	}

	static std::shared_ptr<CActor> FindActorByName(std::string_view Name)
	{
		auto IsNameEqual = [Name](const std::shared_ptr<CActor>& Actor)
		{
			return (Name == Actor->GetName());
		};
		auto Iter = std::find_if(Actors.begin(), Actors.end(), IsNameEqual);
		return (Iter != Actors.end()) ? *Iter : nullptr;
	}

	static bool PreSolve(b2ShapeId ShapeA, b2ShapeId ShapeB, b2Vec2 Point, b2Vec2 Normal, void* Ctx);
	static void GenerateClouds(const std::size_t CloudCount = 7);

	CTestLevel::CTestLevel()
		: IGameInstance(GameSpec)
	{
		Instance = this;
		Actors.clear();

		CRenderer::SetClearColor(FColor::SkyBlue);
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
		LK_VERIFY(Player);
		CreatePlatform();

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

		CWindow::OnResized.Add(this, &CTestLevel::OnWindowResized);
		CWindow::Get()->Maximize();
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
		Tick_Objects(DeltaTime);

		DrawClouds();

		/* Render player. */
		const FPolygon* Polygon = Player->GetBody().TryGetShape<EShape::Polygon>();
		if (Polygon)
		{
			static CTimer PlayerTimer;
			const glm::vec2 PlayerSize = Player->GetSize();

			CRenderer::DrawQuad(
				glm::vec3(Player->GetPosition(), 0.030f),
				PlayerSize,
				*CRenderer::GetTexture(Player->GetTexture()),
				Player->GetSprite().GetUV(),
				FColor::White,
				glm::degrees(Player->GetRotation())
			);
		}

		/* Render level. */
		for (const std::shared_ptr<CActor>& Actor : Actors)
		{
			const FTransformComponent& TC = Actor->GetTransformComponent();
			CRenderer::DrawQuad(
				glm::vec3(Actor->GetPosition(), 0.50f),
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

	bool CTestLevel::Serialize(const std::filesystem::path& Filepath)
	{
		LK_INFO_TAG("TestLevel", "Serializing: {}", Filepath);
		YAML::Emitter Out;
		for (const auto& Actor : Actors)
		{
			Actor->Serialize(Out);
		}

		std::ofstream OutFile(Filepath);
		OutFile << Out.c_str();

		return true;
	}

	bool CTestLevel::Deserialize(const std::filesystem::path& Filepath)
	{
		LK_INFO_TAG("TestLevel", "Deserializing: {}", Filepath);
		LK_ASSERT(std::filesystem::exists(Filepath), "Filepath does not exist: {}", Filepath);
		if (!std::filesystem::exists(Filepath))
		{
			LK_ERROR_TAG("TestLevel", "Filepath does not exist: {}", Filepath);
			return false;
		}

		std::ifstream InputStream(Filepath);
		std::stringstream StringStream;
		StringStream << InputStream.rdbuf();
		const std::string YamlString = StringStream.str();
		LK_DEBUG("YAML:\n{}\n", YamlString);

		YAML::Node Data = YAML::Load(YamlString);

		return true;
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
		Spec.Name = "SpawnPlatform";
		Spec.Position = { 0.0f, -0.72f };
		Spec.Type = EBodyType::Static;
		Spec.Flags = EBodyFlag_PreSolveEvents;

		FPolygon Polygon = {
			.Size = { 2.0f, 0.08f }
		};
		Spec.Shape.emplace<FPolygon>(Polygon);

		std::shared_ptr<CActor> Platform = CActor::Create<CActor>(Spec, ETexture::Metal);
		FTransformComponent& TC = Platform->GetTransformComponent();
		TC.SetScale(Polygon.Size);
	}

	void CTestLevel::CreateTerrain()
	{
		/* Object 1. */
		{
			FBodySpecification Spec;
			Spec.Type = EBodyType::Static;
			Spec.Position = { 3.29f, -0.33f };
			Spec.Flags = EBodyFlag_PreSolveEvents;
			Spec.Name = "Right-Platform";

			FPolygon Polygon = {
				.Size = { 3.04f, 0.12f },
				.Rotation = glm::radians(22.0f),
			};
			Spec.Shape.emplace<FPolygon>(Polygon);

			std::shared_ptr<CActor> Actor = CActor::Create<CActor>(Spec, ETexture::White);
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

		/* Object 5. */
		{
			FBodySpecification Spec;
			Spec.Type = EBodyType::Static;
			Spec.Position = { 0.10f, -1.60f };
			Spec.Flags = EBodyFlag_PreSolveEvents;
			Spec.Name = "Bottom-Platform2";

			FPolygon Polygon = {
				.Size = { 9.60f, 0.22f },
			};
			Spec.Shape.emplace<FPolygon>(Polygon);

			std::shared_ptr<CActor> Actor = CActor::Create<CActor>(Spec, ETexture::White);
			Actor->SetColor(FColor::Gray);
		}

		CSpawner::CreateStaticPolygon("R_Wall-1", { 4.830f, -0.90f }, { 0.10f, 1.20f }, FColor::Convert(RGBA32::Magenta));
	}

	void CTestLevel::Tick_Objects(const float DeltaTime)
	{
		for (const auto& Object : Actors)
		{
			Object->Tick(DeltaTime);
		}

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

		ImGui::Text("Viewport: (%d, %d)", ViewportWidth, ViewportHeight);
		{
			ImGuiViewport* Viewport = ImGui::GetMainViewport();
			ImGui::Text("Main Viewport: (%.1f, %.1f)", Viewport->Size.x, Viewport->Size.y);
		}

		const int Gcd = std::gcd(ViewportWidth, ViewportHeight);
		ImGui::Text("Aspect Ratio: %d/%d", (ViewportWidth / Gcd), (ViewportHeight / Gcd));

		const glm::vec2 HalfSize = GetActiveCamera()->GetHalfSize();
		ImGui::Text("Half Size: (%2.f, %.2f)", HalfSize.x, HalfSize.y);

		const b2Vec2 G = b2World_GetGravity(CPhysicsWorld::GetID());
		ImGui::Text("Gravity: (%.1f, %.1f)", G.x, G.y);

		glm::vec4 ClearColor = CRenderer::GetClearColor();
		if (ImGui::SliderFloat3("Background", &ClearColor.x, 0.0f, 1.0f, "%.2f"))
		{
			CRenderer::SetClearColor(ClearColor);
		}

		ImGui::Dummy(ImVec2(0, 8));
		if (ImGui::Button("Serialize"))
		{
			Serialize(BINARY_DIR "/testlevel.yaml");
		}
		ImGui::Dummy(ImVec2(0, 8));

		ImGui::Text("Actors: %d", Actors.size() + 1);
		UI::Draw::ActorNode(*Player);
		for (auto& Actor : Actors)
		{
			UI::Draw::ActorNode(*Actor);
		}

		ImGui::Dummy(ImVec2(0, 10));
		if (ImGui::Button("Log actors"))
		{
			for (auto& Actor : Actors)
			{
				LK_PRINTLN("{}\nPosition: {}\nRotation: {}\n",
						   Actor->GetName(), Actor->GetPosition(), Actor->GetRotation());
			}
		}

		ImGui::Dummy(ImVec2(0, 10));
		UI_TextureModifier();

		UI::Font::Pop();
		ImGui::End();
	}

	void CTestLevel::UI_Player()
	{
		CBody& PlayerBody = Player->GetBody();
		const FPlayerData& PlayerData = Player->GetData();
		FTransformComponent& TC = Player->GetTransformComponent();

		ImGui::SetNextWindowBgAlpha(UI_BG_ALPHA);
		if (!ImGui::Begin("Player"))
		{
			ImGui::End();
			return;
		}

		glm::vec2 PlayerBodyPos = Player->GetBody().GetPosition();
		ImGui::Text("Position: (%.2f, %.2f)", PlayerBodyPos.x, PlayerBodyPos.y);
		PlayerBodyPos = Player->GetPosition();
		ImGui::Text("TC Position: (%.2f, %.2f)", PlayerBodyPos.x, PlayerBodyPos.y);
		ImGui::Text("Movement State: %s", Enum::ToString(PlayerData.MovementState));
		ImGui::Text("Jump State: %s", PlayerData.bJumping ? "Jumping" : "On ground");

		auto [CurrentSpriteFrame, NextSpriteFrame] = Player->GetCurrentAndNextSpriteFrame();
		ImGui::Text("Current Sprite Frame: %d", CurrentSpriteFrame);
		ImGui::Text("Next Sprite Frame: %d", NextSpriteFrame);
		ImGui::Dummy(ImVec2(0, 4));

		const glm::vec2 PlayerSize = Player->GetSize();
		ImGui::Text("Size: (%.2f, %.2f)", PlayerSize.x, PlayerSize.y);
		ImGui::Text("TC Scale: (%.2f, %.2f)", TC.Scale.x, TC.Scale.y);

		CCamera& Camera = Player->GetCamera();
		ImGui::Text("Camera Zoom: %.2f", Camera.GetZoom());
		const bool CameraLocked = Player->IsCameraLocked();
		ImGui::Text("Camera Lock: %s", CameraLocked ? "Active" : "Not active");
		ImGui::SameLine();
		if (ImGui::Button("Toggle"))
		{
			Player->SetCameraLock(!CameraLocked);
		}

		const glm::vec2 LinearVelocity = PlayerBody.GetLinearVelocity();
		ImGui::Text("Linear Velocity: (%.2f, %.2f)", LinearVelocity.x, LinearVelocity.y);
		const float AngularVelocity = PlayerBody.GetAngularVelocity();
		ImGui::Text("Angular Velocity: %.2f", AngularVelocity);

		ImGui::PushItemWidth(160.0f);
		float PlayerJumpImpulse = Player->GetJumpImpulse();
		ImGui::SliderFloat("Jump Impulse", &PlayerJumpImpulse, 0.0f, 10.0f, "%.5f");
		if (ImGui::IsItemActive()) Player->SetJumpImpulse(PlayerJumpImpulse);

		float PlayerDirForce = Player->GetDirectionForce();
		ImGui::SliderFloat("Direction Force", &PlayerDirForce, 0.0f, 10.0f, "%.5f");
		if (ImGui::IsItemActive()) Player->SetDirectionForce(PlayerDirForce);
		ImGui::Text("Last Direction Force: %.5f", Player->GetLastDirectionForce());

		static float BodyScale = TC.Scale.x;
		ImGui::SliderFloat("Body Scale", &BodyScale, 0.0f, 2.0f, "%.2f");
		ImGui::SameLine();
		if (ImGui::Button("Apply##Scale"))
		{
			PlayerBody.SetScale(BodyScale);
		}

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
			static constexpr const char* PlatformName = "SpawnPlatform";
			if (std::shared_ptr<CActor> Platform = FindActorByName(PlatformName); Platform != nullptr)
			{
				Platform->SetRotation(Platform->GetRotation() + glm::radians(45.0f));
			}
			else
			{
				LK_WARN_TAG("TestLevel", "No actor exists with name: {}", PlatformName);
			}
		}

		ImGui::End();
	}

	void CTestLevel::UI_TextureModifier()
	{
		static const std::array<const char*, CRenderer::MAX_TEXTURES> TextureNames = {
			Enum::ToString(ETexture::White),
			Enum::ToString(ETexture::Background),
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
		if (const std::shared_ptr<CTexture> TextureRef = CRenderer::GetTexture(Texture); TextureRef != nullptr)
		{
			/* Texture preview. */
			ImGui::SameLine(0.0f, 12.0f);
			UI::ShiftCursorY(-4.0f);
			ImGui::Image(
				static_cast<ImU64>(TextureRef->GetID()),
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
				ImGui::Text("Size:%-4s%dx%d", " ", TextureRef->GetWidth(), TextureRef->GetHeight());
				const std::string TrimmedPath = StripPrefix(TextureRef->GetFilePath());
				ImGui::Text("Path:%-4s%s", " ", TrimmedPath.c_str());
				ImGui::Unindent();
			}
		}
		ImGui::Dummy(ImVec2(0, 12));

		{
			UI::FScopedStyle FrameRounding(ImGuiStyleVar_FrameRounding, 10);
			UI::FScopedStyle FramePadding(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
			UI::FScopedStyle FrameBorder(ImGuiStyleVar_FrameBorderSize, 2.0f);
			UI::FScopedColor ButtonCol(ImGuiCol_Button, RGBA32::Titlebar::Default);
			UI::FScopedColor ButtonActiveCol(ImGuiCol_ButtonActive, RGBA32::LightGray);
			UI::FScopedColor ButtonHoveredCol(ImGuiCol_ButtonHovered, RGBA32::SelectionMuted);

			UI::ShiftCursorX(35.0f);
			ImGui::Text("Wrap");

			ImGui::SameLine((Avail.x * 0.50f) - (ItemWidth * 0.50f) + ButtonPaddingY);
			UI::ShiftCursorY(-ButtonPaddingY);
			if (ImGui::Button("Clamp", ButtonSize))
			{
				CRenderer::GetTexture(Texture)->SetWrap(ETextureWrap::Clamp);
			}
			ImGui::SameLine(0.0f, ButtonPaddingY);
			UI::ShiftCursorY(-ButtonPaddingY);
			if (ImGui::Button("Repeat", ButtonSize))
			{
				CRenderer::GetTexture(Texture)->SetWrap(ETextureWrap::Repeat);
			}

			ImGui::Dummy(ImVec2(0, 6));

			UI::ShiftCursorX(35.0f);
			ImGui::Text("Filter");

			ImGui::SameLine((Avail.x * 0.50f) - (ItemWidth * 0.50f) + ButtonPaddingY);
			UI::ShiftCursorY(-ButtonPaddingY);
			if (ImGui::Button("Linear", ButtonSize))
			{
				CRenderer::GetTexture(Texture)->SetFilter(ETextureFilter::Linear);
			}

			ImGui::SameLine(0.0f, ButtonPaddingY);
			UI::ShiftCursorY(-ButtonPaddingY);
			if (ImGui::Button("Nearest", ButtonSize))
			{
				CRenderer::GetTexture(Texture)->SetFilter(ETextureFilter::Nearest);
			}
		}
	}

	void CTestLevel::DrawBackground() const
	{
		const CTexture& BgTexture = *CRenderer::GetTexture(ETexture::Background);
		const glm::vec2 HalfSize = GetActiveCamera()->GetHalfSize();
		const glm::vec2 BgSize(HalfSize.x * 3.0f, HalfSize.y * 4.0f);
		CRenderer::DrawQuad({ 0.0f, 0.0f, 0.0f }, BgSize, BgTexture, FColor::White);
	}

	void CTestLevel::DrawClouds() const
	{
		const CTexture& Texture = *CRenderer::GetTexture(ETexture::Cloud);
		for (const FCloud& Cloud : Clouds)
		{
			CRenderer::DrawQuad(Cloud.Position, Cloud.Size, Texture, FColor::White);
		}
	}

	void CTestLevel::OnWindowResized(const uint16_t InWidth, const uint16_t InHeight)
	{
		ViewportWidth = InWidth;
		ViewportHeight = InHeight;
		LK_TRACE_TAG("TestLevel", "Window resized: ({}, {})", ViewportWidth, ViewportHeight);
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

	void GenerateClouds(const std::size_t CloudCount)
	{
		static constexpr float MinX = -3.0f;
		static constexpr float MaxX = 3.0f;
		static constexpr float MinY = 0.20f;
		static constexpr float MaxY = 1.0f;
		static constexpr float MinScale = 0.90f;
		static constexpr float MaxScale = 1.15f;

		Clouds.clear();
		Clouds.reserve(CloudCount);

		std::random_device RandomDevice;
		std::mt19937 Engine(RandomDevice());

		std::uniform_real_distribution<float> DistX(MinX, MaxX);
		std::uniform_real_distribution<float> DistY(MinY, MaxY);
		std::uniform_real_distribution<float> DistScale(MinScale, MaxScale);

		constexpr glm::vec2 BaseSize = glm::vec2(1.0f, 0.80f);
		for (int Idx = 0; Idx < CloudCount; Idx++)
		{
			const float X = DistX(Engine);
			const float Y = DistY(Engine);
			const float Scale = DistScale(Engine);

			FCloud Cloud;
			Cloud.Position = glm::vec3(X, Y, Math::Randomize(0.0f, 0.25f));
			Cloud.Size = BaseSize * Scale;
			Clouds.push_back(Cloud);
			LK_DEBUG_TAG("TestLevel", "Cloud {}: {}, {}", Idx, Cloud.Position, Cloud.Size);
		}
	}

}
