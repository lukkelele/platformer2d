#include "testlevel.h"

#include <fstream>
#include <istream>
#include <numeric>

#include "core/window.h"
#include "core/timer.h"
#include "core/selectioncontext.h"
#include "core/string.h"
#include "core/input/keyboard.h"
#include "core/input/mouse.h"
#include "core/math/math.h"
#include "game/player.h"
#include "game/spawner.h"
#include "renderer/renderer.h"
#include "renderer/debugrenderer.h"
#include "renderer/ui/ui.h"
#include "renderer/ui/widgets.h"
#include "physics/body.h"
#include "physics/physicsworld.h"
#include "physics/ray.h"
#include "serialization/serialization.h"

#define PLAYER_SHAPE_CAPSULE 0

namespace platformer2d::Level {

	namespace
	{
		/*************************************
		 *        GAME SPECIFICATION
		/*************************************/
		const FGameSpecification GameSpec = {
			.LevelFilepath = std::filesystem::path(LEVELS_DIR "/testlevel.yaml"),
			.Name = "TestLevel",
			.Gravity = { 0.0f, -5.0f },
			.Zoom = 0.32f,
			.PlayerBody = {
				.Type = EBodyType::Dynamic,
				.Shape = FPolygon{
					.Size = { 0.20f, 0.24f },
					.Radius = 0.12f,
					.Rotation = glm::radians(0.0f),
				},
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

		const std::array<const char*, CRenderer::MAX_TEXTURES> TextureNames = {
			Enum::ToString(ETexture::White),
			Enum::ToString(ETexture::Background),
			Enum::ToString(ETexture::Player),
			Enum::ToString(ETexture::Metal),
			Enum::ToString(ETexture::Bricks),
			Enum::ToString(ETexture::Wood),
		};

		char ActorNameBuf[128] = { 0 };
		std::array<glm::vec2, 2> ViewportBounds;
	}

	static bool PreSolve(b2ShapeId ShapeA, b2ShapeId ShapeB, b2Vec2 Point, b2Vec2 Normal, void* Ctx);
	static void GenerateClouds(const std::size_t CloudCount = 7);

	CTestLevel::CTestLevel()
		: IGameInstance(this, GameSpec)
	{
		CRenderer::SetClearColor(FColor::SkyBlue);
	}

	void CTestLevel::Initialize()
	{
		LK_DEBUG_TAG("TestLevel", "Initialize");
		LK_ASSERT(Player == nullptr);

		Scene = std::make_shared<CScene>("TestLevel");

		CActor::OnActorCreated.Add([&](const LUUID Handle, std::weak_ptr<CActor> ActorRef)
		{
			if (std::shared_ptr<CActor> Actor = ActorRef.lock(); Actor != nullptr)
			{
				LK_TRACE_TAG("TestLevel", "OnActorCreated: {} ({})", Actor->GetName(), Handle);
				LK_ASSERT(Scene);
				const auto& Actors = Scene->GetActors(); /* @fixme */
				std::snprintf(ActorNameBuf, sizeof(ActorNameBuf), "Actor-%lld", Actors.size() + 2);

				std::string_view ActorName = Actor->GetName();
				if (ActorName.find("Rotating") != std::string::npos)
				{
					/* @fixme: Temporary fix until serialization can take effect parameters. */
					RotatingPlatform = Actor;
				}
			}
		});

		/**
		 * Bind to lambda because the delegate expects a void return type,
		 * which DeleteActor isn't.
		 */
		CActor::OnActorMarkedForDeletion.Add([&](const LUUID Handle)
		{
			std::snprintf(ActorNameBuf, sizeof(ActorNameBuf), "Actor-%lld", Scene->GetActors().size() + 2);
		});

		const FGameSpecification& Spec = GetSpecification();
		CPhysicsWorld::SetGravity(Spec.Gravity);

		CreatePlayer();
		LK_VERIFY(Player);

#if 1
		Deserialize(GameSpec.LevelFilepath);
#else
		CreateTerrain();
		CreatePlatform();
#endif

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
		CWindow* Window = CWindow::Get();
		Window->Maximize();
		ViewportBounds[0] = { 0.0f, 0.0f };
		ViewportBounds[1] = Window->GetSize();
	}

	void CTestLevel::Destroy()
	{
		LK_TRACE_TAG("TestLevel", "Destroy");
		Serialize(GameSpec.LevelFilepath);

		LK_DEBUG_TAG("TestLevel", "Release level resources");
		Player.reset();
		Scene.reset();
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
		Scene->Tick(DeltaTime);
		Tick_Objects(DeltaTime);

#if 1
		const uint16_t Picked = PickSceneAtMouse(Scene, SelectionData);
		if (Picked > 0)
		{
			const FSceneSelectionEntry& Selected = SelectionData.at(0);
			if (Selected.Ref)
			{
				UI::DrawGizmo(ImGuizmo::TRANSLATE, *Selected.Ref, Camera.GetViewMatrix(), Camera.GetProjectionMatrix());
			}
		}
#else
		const uint16_t Hits = RaycastScene(Scene, SelectionData);
		if (Hits > 0)
		{
			const FSceneSelectionEntry& Selected = SelectionData.at(0);
			if (Selected.Ref)
			{
				UI::DrawGizmo(ImGuizmo::TRANSLATE, *Selected.Ref, Camera.GetViewMatrix(), Camera.GetProjectionMatrix());
			}
		}
#endif

		DrawClouds();

		/* Render player. */
		const FPolygon* Polygon = Player->GetBody().TryGetShape<EShape::Polygon>();
		if (Polygon)
		{
			CRenderer::DrawQuad(
				glm::vec3(Player->GetPosition(), 0.030f),
				Player->GetSize(),
				*CRenderer::GetTexture(Player->GetTexture()),
				Player->GetSprite().GetUV(),
				FColor::White,
				glm::degrees(Player->GetRotation())
			);
		}

		/* Render level. */
		for (const std::shared_ptr<CActor>& Actor : Scene->GetActors())
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
			static constexpr glm::vec4 OverlayColor = { 0.10f, 0.10f, 0.10f, 0.85f };
			CRenderer::DrawQuad(glm::vec3(0.0f, 0.0f, 2.0f), { ViewportWidth, ViewportHeight }, OverlayColor);
		}
	}

	CCamera* CTestLevel::GetActiveCamera() const
	{
		return (Player ? &Player->GetCamera() : nullptr);
	}

	CPlayer* CTestLevel::GetPlayer(std::size_t Idx) const
	{
		LK_ASSERT(Idx == 0, "TestLevel only supports 1 player");
		return Player.get();
	}

	static inline glm::vec2 GetMouseViewportSpace()
	{
		auto [MouseX, MouseY] = CMouse::GetPos();
		MouseX -= ViewportBounds[0].x;
		MouseY -= ViewportBounds[0].y;
		const float ViewportWidth = ViewportBounds[1].x - ViewportBounds[0].x;
		const float ViewportHeight = ViewportBounds[1].y - ViewportBounds[0].y;

		return glm::vec2(
			(MouseX / ViewportWidth) * 2.0f - 1.0f,
			((MouseY / ViewportHeight) * 2.0f - 1.0f) * -1.0f
		);
	}

	static inline glm::vec2 GetMouseWorldSpace(const CCamera& Camera)
	{
		const glm::vec2 MousePos = GetMouseViewportSpace();
		if ((MousePos.x < -1.0f) || (MousePos.x > 1.0f) || (MousePos.y < -1.0f) || (MousePos.y > 1.0f))
		{
			return glm::vec2(std::numeric_limits<float>::quiet_NaN());
		}

		const glm::vec4 ClipPos = glm::vec4(MousePos.x, MousePos.y, 0.0f, 1.0f);
		const glm::mat4 InvViewProj = glm::inverse(Camera.GetProjectionMatrix() * Camera.GetViewMatrix());
		glm::vec4 WorldPos = InvViewProj * ClipPos;
		if (WorldPos.w != 0.0f)
		{
			WorldPos /= WorldPos.w;
		}

		return WorldPos;
	}

	uint16_t CTestLevel::RaycastScene(std::shared_ptr<CScene> TargetScene, std::vector<FSceneSelectionEntry>& Selected)
	{
		static FRayCast RayData;
		Selected.clear();

		const glm::vec2 MousePos = GetMouseViewportSpace();
		if ((MousePos.x < -1.0f) || (MousePos.x > 1.0f) || (MousePos.y < -1.0f) || (MousePos.y > 1.0f))
		{
			return 0;
		}

		const CCamera& Camera = *GetActiveCamera();
		Physics::CastRay(
			RayData,
			Camera.GetPosition(),
			Camera.GetViewMatrix(),
			Camera.GetProjectionMatrix(),
			MousePos.x,
			MousePos.y
		);

		for (const auto& Actor : TargetScene->GetActors())
		{
			const glm::vec2 Pos = Actor->GetPosition();
			const glm::vec2 Size = Actor->GetBody().GetSize();
			const glm::vec2 HalfSize = Size * 0.50f;
			const glm::vec2 BoxMin = Pos - HalfSize;
			const glm::vec2 BoxMax = Pos + HalfSize;

			float T = 0.0f;
			if (Physics::RaycastAABB(RayData, BoxMin, BoxMax, T))
			{
				Selected.push_back(FSceneSelectionEntry{ Actor->GetHandle(), Actor.get(), T });
				CDebugRenderer::DrawRayHit(RayData, T); /* @todo: Toggle for this */
			}
		}

		if (Selected.empty())
		{
			return 0;
		}

		std::sort(Selected.begin(), Selected.end(), [](auto& Lhs, auto& Rhs) { return Lhs.Distance < Rhs.Distance; });
		return static_cast<uint16_t>(Selected.size());
	}

	uint16_t CTestLevel::PickSceneAtMouse(std::shared_ptr<CScene> TargetScene, std::vector<FSceneSelectionEntry>& Selected)
	{
		Selected.clear();
		const CCamera& Camera = *GetActiveCamera();
		const glm::vec2 MouseWorld = GetMouseWorldSpace(Camera);
		if (!std::isfinite(MouseWorld.x) || !std::isfinite(MouseWorld.y))
		{
			return 0;
		}

		for (const auto& Actor : TargetScene->GetActors())
		{
			const glm::vec2 Pos = Actor->GetPosition();
			const glm::vec2 Size = Actor->GetBody().GetSize();
			const float Rotation = Actor->GetRotation();
			if (Math::IsPointInPolygon(MouseWorld, Pos, Size, Rotation))
			{
				FSceneSelectionEntry Entry{};
				Entry.Handle = Actor->GetHandle();
				Entry.Ref = Actor.get();

				const glm::vec2 Delta = MouseWorld - Pos;
				Entry.Distance = glm::length(Delta);

				Selected.push_back(Entry);
			}
		}

		if (Selected.empty())
		{
			return 0;
		}

		std::sort(Selected.begin(), Selected.end(), [](const auto& Lhs, const auto& Rhs) { return Lhs.Distance < Rhs.Distance; });
		return static_cast<uint16_t>(Selected.size());
	}

	void CTestLevel::RenderUI()
	{
		UI_Level();
		UI_Player();
	}

	bool CTestLevel::Serialize(const std::filesystem::path& OutFile) const
	{
		LK_INFO_TAG("TestLevel", "Serialize: {}", OutFile);
		YAML::Emitter Out;

		Out << YAML::BeginMap; /* Level */
		Out << YAML::Key << "Level" << YAML::Value << Name;

		const std::filesystem::path ScenePath = Scene->GetFilepath();
		Out << YAML::Key << "Scene" << YAML::Value << ScenePath;

		/* Physics */
		Out << YAML::Key << "Physics";
		Out << YAML::BeginMap;
		Out << YAML::Key << "Gravity" << YAML::Value << CPhysicsWorld::GetGravity();
		Out << YAML::EndMap;
		/* ~ Physics */

		Out << YAML::EndMap; /* ~Level */

		std::ofstream File(OutFile);
		File << Out.c_str();

		/* Save scene to its own file. */
		LK_ASSERT(Scene);
		Scene->Serialize(ScenePath);

		return true;
	}

	bool CTestLevel::Deserialize(const std::filesystem::path& Filepath)
	{
		LK_INFO_TAG("TestLevel", "Deserialize: {}", StringUtils::GetPathRelativeToProject(Filepath));
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

		const YAML::Node Data = YAML::Load(YamlString);

		/* Load the scene. */
		const YAML::Node SceneNode = Data["Scene"];
		LK_ASSERT(!SceneNode.IsNull());
		if (SceneNode.IsNull())
		{
			LK_ERROR_TAG("TestLevel", "Scene node is missing in YAML");
			return false;
		}

		const std::filesystem::path SceneFilepath = SceneNode.as<std::filesystem::path>();
		LK_INFO_TAG("TestLevel", "Loading scene: {}", StringUtils::GetPathRelativeToProject(SceneFilepath));
		const bool SceneDeserialized = Scene->Deserialize(SceneFilepath);
		if (!SceneDeserialized)
		{
			LK_FATAL_TAG("TestLevel", "Failed to deserialize scene");
			return false;
		}

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

			std::shared_ptr<CActor> Actor = CActor::Create<CActor>(Spec, ETexture::White, FColor::LightGreen);
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

			std::shared_ptr<CActor> Actor = CActor::Create<CActor>(Spec, ETexture::White, FColor::Convert(RGBA32::Magenta));
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

			std::shared_ptr<CActor> Actor = CActor::Create<CActor>(Spec, ETexture::White, FColor::Convert(RGBA32::DarkCyan));
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

			std::shared_ptr<CActor> Actor = CActor::Create<CActor>(Spec, ETexture::White, FColor::Gray);
		}

		CSpawner::CreateStaticPolygon(
			"R_Wall-1",
			{ 4.830f, -0.90f }, /* Pos */
			{ 0.10f, 1.20f },   /* Size */
			FColor::Convert(RGBA32::Magenta)
		);
	}

	void CTestLevel::Tick_Objects(const float DeltaTime)
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

		ImGui::Dummy(ImVec2(0, 8));

		glm::vec4 ClearColor = CRenderer::GetClearColor();
		if (ImGui::SliderFloat3("Background", &ClearColor.x, 0.0f, 1.0f, "%.2f"))
		{
			CRenderer::SetClearColor(ClearColor);
		}

		ImGui::Dummy(ImVec2(0, 8));

		const glm::vec2 MousePos = GetMouseViewportSpace();
		ImGui::Text("Mouse: (%.2f, %.2f)", MousePos.x, MousePos.y);

		std::string Selected = "None";
		if (!SelectionData.empty())
		{
			if (CActor* Ref = SelectionData[0].Ref; Ref != nullptr)
			{
				Selected = Ref->GetName();
			}
		}
		ImGui::Text("Selected: %s", Selected.c_str());

		ImGui::Dummy(ImVec2(0, 8));
		{
			UI::FScopedStyle ButtonRounding(ImGuiStyleVar_FrameRounding, 8.0f);
			if (ImGui::Button("Serialize"))
			{
				Serialize(GameSpec.LevelFilepath);
			}
			ImGui::SameLine();
			if (ImGui::Button("Deserialize"))
			{
				Deserialize(GameSpec.LevelFilepath);
			}
		}
		ImGui::Dummy(ImVec2(0, 8));

		const auto& Actors = Scene->GetActors();
		ImGui::Text("Actors: %d", Actors.size() + 1);
		UI::Draw::ActorNode(*Player);
		for (auto& Actor : Actors)
		{
			UI::Draw::ActorNode(*Actor);
		}

		ImGui::Dummy(ImVec2(0, 10));
		ImGui::Separator();
		ImGui::PushID("CreatorMenu");
		{
			const ImVec2 Avail = ImGui::GetContentRegionAvail();

			UI::Font::Push(EFont::SourceSansPro, EFontSize::Header, EFontModifier::Bold);
			static constexpr const char* CreatorMenuLabel = "Creator";
			static const ImVec2 LabelSize = ImGui::CalcTextSize(CreatorMenuLabel);
			UI::ShiftCursorX((0.50f * Avail.x) - LabelSize.x);
			ImGui::Text("Creator Menu");
			UI::Font::Pop();

			ImGui::BeginTable("##VectorControl", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoClip);
			ImGui::TableSetupColumn("LabelColumn", 0, 140.0f);
			ImGui::TableSetupColumn("ValueColumn", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, ImGui::GetContentRegionAvail().x - 140.0f);

			ImGui::TableNextRow();
			/* Column 0 */
			ImGui::TableSetColumnIndex(0);
			UI::ShiftCursor(17.0f, 7.0f);
			ImGui::Text("Name");

			/* Column 1 */
			ImGui::TableSetColumnIndex(1);
			UI::ShiftCursor(7.0f, 0.0f);
			ImGui::SetNextItemWidth(160.0f);
			ImGui::InputText("##ActorName", ActorNameBuf, LK_ARRAYSIZE(ActorNameBuf));

			ImGui::TableNextRow();
			static glm::vec2 Pos = { 0.0f, 0.0f };
			UI::Draw::Vec2Control("Position", Pos, 0.0f, 0.010f, -100.0f, 100.0f);

			ImGui::TableNextRow();
			static glm::vec2 Size = { 0.20f, 0.20f };
			UI::Draw::Vec2Control("Size", Size, 1.0f, 0.010f, 0.010f, 2.0f);

			ImGui::EndTable();

			static std::size_t SelectedTextureIdx = 0;
			UI_TextureDropDown(SelectedTextureIdx);

			UI::ShiftCursorX(32.0f);
			{
				static constexpr ImVec2 ButtonSize = ImVec2(82, 42);
				UI::FScopedFont Font(UI::Font::Get(EFont::SourceSansPro, EFontSize::Regular, EFontModifier::Bold));
				UI::FScopedStyle ButtonFrame(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
				UI::FScopedStyle ButtonRounding(ImGuiStyleVar_FrameRounding, 8);
				UI::FScopedColorStack ButtonColours(
					ImGuiCol_ButtonHovered, RGBA32::DarkGreen,
					ImGuiCol_ButtonActive, RGBA32::NiceGreen
				);

				if (ImGui::Button("Create", ButtonSize))
				{
					if (!Scene->DoesActorExist(ActorNameBuf) && ((Size.x > 0.0f) && (Size.y > 0.0f)))
					{
						CSpawner::CreateStaticPolygon(ActorNameBuf, Pos, Size, FColor::Convert(RGBA32::Magenta));
					}
					else
					{
						LK_ERROR_TAG("TestLevel", "Actor already exists with name: \"{}\"", ActorNameBuf);
					}
				}
			}
		}
		ImGui::PopID();
		ImGui::Separator();
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
			if (std::shared_ptr<CActor> Platform = Scene->FindActor(PlatformName); Platform != nullptr)
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

	void CTestLevel::UI_TextureDropDown(std::size_t& SelectedIdx)
	{
		static constexpr float ButtonPaddingY = 7.0f;
		static constexpr ImVec2 ButtonSize(84, 42);
		static constexpr float ItemWidth = 2.0f * ButtonSize.x;

		LK_ASSERT((SelectedIdx >= 0) && (SelectedIdx < TextureNames.size()));
		static const char* SelectedTexture = TextureNames[SelectedIdx];

		ImGui::Dummy(ImVec2(0, 6));
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
		if (ImGui::BeginCombo("##Texture", TextureNames[SelectedIdx]))
		{
			SelectedTexture = TextureNames[SelectedIdx];
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
					SelectedIdx = Idx;
				}
			}

			ImGui::EndCombo();
		}
	}

	void CTestLevel::UI_TextureModifier()
	{
		static constexpr float ButtonPaddingY = 7.0f;
		static constexpr ImVec2 ButtonSize(84, 42);
		static constexpr float ItemWidth = 2.0f * ButtonSize.x;

		ImGui::Dummy(ImVec2(0, 12));
		static std::size_t SelectedTextureIdx = 0;
		UI_TextureDropDown(SelectedTextureIdx);

		const ImVec2 Avail = ImGui::GetContentRegionAvail();

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

	void CTestLevel::DeserializeActors(const YAML::Node& ActorsNode)
	{
		LK_INFO_TAG("TestLevel", "Deserializing actors");
		for (const YAML::Node& Node : ActorsNode)
		{
			LK_ASSERT(Node["ID"] && Node["Name"] && Node["Texture"] && Node["Color"] && Node["TransformComponent"]);
			const LUUID ActorHandle = Node["ID"].as<LUUID>();
			const std::string ActorName = Node["Name"].as<std::string>();
			const ETexture ActorTexture = static_cast<ETexture>(Node["Texture"].as<int>());
			const glm::vec4 ActorColor = Node["Color"].as<glm::vec4>();
			LK_TRACE_TAG("TestLevel", "Handle={} Name={}", ActorHandle, ActorName);

			const YAML::Node TCNode = Node["TransformComponent"];
			FTransformComponent TC;
			Serialization::Deserialize(TC, TCNode);

			FBodySpecification BodySpec;
			BodySpec.Name = ActorName;
			if (const YAML::Node BodyNode = Node["Body"]; !BodyNode.IsNull())
			{
				Serialization::Deserialize(BodySpec, BodyNode);
				LK_TRACE("{}", CBody::ToString(BodySpec));
			}

			if (!Scene->DoesActorExist(ActorHandle))
			{
				std::shared_ptr<CActor> Actor = CActor::Create<CActor>(ActorHandle, BodySpec, ActorTexture, ActorColor);
			}
			else
			{
				LK_ERROR_TAG("TestLevel", "Duplicate actors found during deserialization with handle {}", ActorHandle);
			}
		}
	}

	bool PreSolve(b2ShapeId ShapeA, b2ShapeId ShapeB, b2Vec2 Point, b2Vec2 Normal, void* Ctx)
	{
		LK_ASSERT(b2Shape_IsValid(ShapeA) && b2Shape_IsValid(ShapeB));
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
