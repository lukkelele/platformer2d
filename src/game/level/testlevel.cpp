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
#include "renderer/ui/selectionpanel.h"
#include "physics/body.h"
#include "physics/physicsworld.h"
#include "physics/ray.h"
#include "serialization/serialization.h"

namespace platformer2d::Level {

	namespace
	{
		/*************************************
		 *        GAME SPECIFICATION
		 *************************************/
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
				.Flags = EBodyFlag::EBodyFlag_SensorEvents,
				.MotionLock = EMotionLock_Z,
			},
		};

		constexpr float UI_BG_ALPHA = 0.70f;
		constexpr const char* UI_ID_LEVEL = "Level";
		constexpr const char* UI_ID_PLAYER = "Player";

		std::weak_ptr<CActor> SelectedActor;
		std::weak_ptr<CActor> RotatingPlatform;

		const std::array<const char*, CRenderer::MAX_TEXTURES> TextureNames = {
			Enum::ToString(ETexture::White),
			Enum::ToString(ETexture::Background),
			Enum::ToString(ETexture::Player),
			Enum::ToString(ETexture::Metal),
			Enum::ToString(ETexture::Bricks),
			Enum::ToString(ETexture::Wood),
		};

		int Gizmo = ImGuizmo::OPERATION::TRANSLATE;
		bool bRaycastScene = false;

		std::array<glm::vec2, 2> EditorViewportBounds;
	}

	static bool PreSolve(b2ShapeId ShapeA, b2ShapeId ShapeB, b2Vec2 Point, b2Vec2 Normal, void* Ctx);

	static glm::vec2 GetEditorViewportSize()
	{
		return glm::vec2(
			EditorViewportBounds[1].x - EditorViewportBounds[0].x,
			EditorViewportBounds[1].y - EditorViewportBounds[0].y
		);
	}

	static void UpdateInputBuffer(std::size_t Count)
	{
		std::snprintf(UI::ActorAttr.NameBuf.data(), sizeof(UI::ActorAttr.NameBuf), "Actor-%lld", Count + 2);
	}

	CTestLevel::CTestLevel()
		: CGameInstance(this, GameSpec)
	{
		CRenderer::SetClearColor(FColor::SkyBlue);
	}

	void CTestLevel::Initialize()
	{
		LK_DEBUG_TAG("TestLevel", "Initialize");
		LK_ASSERT(Player == nullptr);

		Scene = std::make_shared<CScene>("TestLevel");

		CScene::OnActorCreated.Add([&](const LUUID Handle, std::weak_ptr<CActor> ActorRef)
		{
			if (std::shared_ptr<CActor> Actor = ActorRef.lock(); Actor != nullptr)
			{
				LK_TRACE_TAG("TestLevel", "OnActorCreated: {} ({})", Actor->GetName(), Handle);
				LK_ASSERT(Scene);
				UpdateInputBuffer(Scene->GetActors().size());

#if 0 /* @fixme: Remove once the creator menu can add new components */
				std::string_view ActorName = Actor->GetName();
				if (ActorName.find("Rotating") != std::string::npos)
				{
					/* @fixme: Temporary fix until serialization can take effect parameters. */
					RotatingPlatform = Actor;
					auto& EC = Actor->AddComponent<FEffectComponent>();
					FEffectInstance Effect;
					Effect.Type = EEffectType::Rotate;
					FRotateEffect Rotate;
					Rotate.AngularSpeedDegPerSecond = 10.0f;
					Effect.Data = Rotate;
					EC.Effects.push_back(Effect);
				}
#endif
			}
		});

		CScene::OnActorDeleted.Add([&](const LUUID Handle)
		{
			LK_DEBUG_TAG("TestLevel", "OnActorDeleted: {}", Handle);
			UpdateInputBuffer(Scene->GetActors().size());
			UI::Draw::OnActorDeleted(Handle);
		});

		const FGameSpecification& Spec = GetSpecification();
		CPhysicsWorld::SetGravity(Spec.Gravity);
		CPhysicsWorld::OnSensorBeginEvent.Add(this, &CTestLevel::OnSensorBeginEvent);
		CPhysicsWorld::OnSensorEndEvent.Add(this, &CTestLevel::OnSensorEndEvent);

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
		UpdateViewportBounds();

		CKeyboard::OnKeyPressed.Add(this, &CTestLevel::OnKeyPressed);
		CMouse::OnButtonPressed.Add(this, &CTestLevel::OnMouseButtonPressed);
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

	void CTestLevel::Tick(const float InDeltaTime)
	{
		DeltaTime = InDeltaTime;
		CCamera& Camera = Player->GetCamera();
#if 0
		const glm::vec2 EditorViewport = GetEditorViewportSize();
		ViewportWidth = EditorViewport.x;
		ViewportHeight = EditorViewport.y;
		Camera.SetViewportSize(ViewportWidth, ViewportHeight);
#else
		Camera.SetViewportSize(EditorViewportWidth, EditorViewportHeight);
#endif
		CRenderer::BeginScene(Camera);

		Player->Tick(DeltaTime);
		Scene->Tick(DeltaTime);

		if (bRaycastScene)
		{
			RaycastScene();
		}

		/* Render player. */
		CRenderer::DrawQuad(
			Player->GetPosition(),
			Player->GetSize(),
			*CRenderer::GetTexture(Player->GetTexture()),
			Player->GetSprite().GetUV(),
			FColor::White,
			glm::degrees(Player->GetRotation())
		);

		/* Render level. */
		for (const std::shared_ptr<CActor>& Actor : Scene->GetActors())
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
	}

	CCamera* CTestLevel::GetActiveCamera() const
	{
		return (Player ? &Player->GetCamera() : nullptr);
	}

	CPlayer* CTestLevel::GetPlayer(const std::size_t Idx) const
	{
		LK_ASSERT(Idx == 0, "TestLevel only supports 1 player");
		return Player.get();
	}

	uint16_t CTestLevel::RaycastScene(std::shared_ptr<CScene> TargetScene, std::vector<FHitResult>& HitResults)
	{
		static FRayCast RayData;
		HitResults.clear();

		const glm::vec2 MousePos = GetMouseInViewportSpace();
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
				HitResults.push_back(FHitResult{ Actor->GetHandle(), Actor, T });

				if (Config.Debug.bDrawRayHits)
				{
					CDebugRenderer::DrawRayHit(RayData, T);
				}
			}
		}

		if (HitResults.empty())
		{
			return 0;
		}

		std::sort(HitResults.begin(), HitResults.end(), [](auto& Lhs, auto& Rhs) { return Lhs.Distance < Rhs.Distance; });
		return static_cast<uint16_t>(HitResults.size());
	}

	uint16_t CTestLevel::PickSceneAtMouse(std::shared_ptr<CScene> TargetScene, std::vector<FHitResult>& HitResults)
	{
		HitResults.clear();
		const CCamera& Camera = *GetActiveCamera();
		const glm::vec2 MouseWorld = GetMouseInWorldSpace(Camera);
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
				FHitResult Entry{};
				Entry.Handle = Actor->GetHandle();
				Entry.Ref = Actor;

				const glm::vec2 Delta = MouseWorld - Pos;
				Entry.Distance = glm::length(Delta);

				HitResults.push_back(Entry);
			}
		}

		if (HitResults.empty())
		{
			return 0;
		}

		std::sort(HitResults.begin(), HitResults.end(), [](const auto& Lhs, const auto& Rhs) { return Lhs.Distance < Rhs.Distance; });
		return static_cast<uint16_t>(HitResults.size());
	}

	void CTestLevel::RenderUI()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		UI::Begin(UI::PanelID::CoreViewport, nullptr, UI::CoreViewportFlags);
		ImGui::PopStyleVar(2);

		UI_PrepareEditorViewport();
		UI::Begin(UI::PanelID::EditorViewport, nullptr, UI::EditorViewportFlags);
		{
			UpdateEditorViewportBounds();
			UI_ViewportTexture();

			UI_Level();
			UI_Player();

			UI::SelectionPanel();

			if (std::shared_ptr<CActor> SelectedRef = SelectedActor.lock(); SelectedRef != nullptr)
			{
				if (ImGuiWindow* Window = ImGui::FindWindowByName(UI::PanelID::EditorViewport))
				{
					ImGui::Begin(Window->Name, nullptr, UI::CoreViewportFlags | ImGuiWindowFlags_NoScrollbar);

					CCamera& Camera = Player->GetCamera();
					if (UI::DrawGizmo(Gizmo, *SelectedRef, Camera.GetViewMatrix(), Camera.GetProjectionMatrix()))
					{
						Player->SetAwake(true);
					}

					ImGui::End();
				}
			}
		}
		UI::End(); /* ~EditorViewport */

		UI::End(); /* ~Viewport */
	}

	bool CTestLevel::Serialize(const std::filesystem::path& OutFile) const
	{
		LK_INFO_TAG("TestLevel", "Serialize: {}", OutFile);
		YAML::Emitter Out;

		Out << YAML::BeginMap; /* Level */
		Out << YAML::Key << "Level" << YAML::Value << Name;

		std::filesystem::path ScenePath = Scene->GetFilepath();
		LK_DEBUG("Scene path: {}", ScenePath);
		Out << YAML::Key << "Scene" << YAML::Value << Core::GetRelativeFromProject(ScenePath);

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

	void CTestLevel::UpdateEditorViewportBounds()
	{
		ImGuiWindow* Window = ImGui::GetCurrentWindow();
		const ImVec2 WindowPos = Window->Pos;

		const ImVec2 RegionMin = ImGui::GetWindowContentRegionMin();
		const ImVec2 RegionMax = ImGui::GetWindowContentRegionMax();

		EditorViewportBounds[0] = {
			WindowPos.x + RegionMin.x,
			WindowPos.y + RegionMin.y
		};

		EditorViewportBounds[1] = {
			WindowPos.x + RegionMax.x,
			WindowPos.y + RegionMax.y
		};

		const float VpWidth = EditorViewportBounds[1].x - EditorViewportBounds[0].x;
		const float VpHeight = EditorViewportBounds[1].y - EditorViewportBounds[0].y;

		if ((EditorViewportWidth != static_cast<uint16_t>(VpWidth)) ||
			(EditorViewportHeight != static_cast<uint16_t>(VpHeight)))
		{
			EditorViewportWidth  = static_cast<uint16_t>(VpWidth);
			EditorViewportHeight = static_cast<uint16_t>(VpHeight);
			CRenderer::GetViewportFramebuffer()->Resize(EditorViewportWidth, EditorViewportHeight);
		}
	}

	void CTestLevel::UpdateViewportBounds()
	{
		ViewportBounds[0] = { 0.0f, 0.0f };
		if (CWindow* Window = CWindow::Get(); Window != nullptr)
		{
			ViewportBounds[1] = Window->GetSize();
		}
		else
		{
			LK_WARN_TAG("TestLevel", "[{}] No window reference", GetSpecification().Name);
			ViewportBounds[1] = { 0.0f, 0.0f };
		}
	}

	glm::vec2 CTestLevel::GetMouseInViewportSpace()
	{
		auto [MouseX, MouseY] = CMouse::GetPos();
		MouseX -= EditorViewportBounds[0].x;
		MouseY -= EditorViewportBounds[0].y;
		const float VpWidth = EditorViewportBounds[1].x - EditorViewportBounds[0].x;
		const float VpHeight = EditorViewportBounds[1].y - EditorViewportBounds[0].y;

		return glm::vec2(
			(MouseX / static_cast<float>(VpWidth)) * 2.0f - 1.0f,
			((MouseY / static_cast<float>(VpHeight)) * 2.0f - 1.0f) * -1.0f
		);
	}

	glm::vec2 CTestLevel::GetMouseInWorldSpace(const CCamera& Camera)
	{
		const glm::vec2 MousePos = GetMouseInViewportSpace();
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

	void CTestLevel::CreatePlayer()
	{
		const FGameSpecification& Spec = GetSpecification();
		Player = std::make_shared<CPlayer>(Spec.PlayerBody, ETexture::Player);
		CPhysicsWorld::SetPreSolve(PreSolve, Player.get());

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

		std::shared_ptr<CActor> Platform = Scene->Create<CActor>(Spec, ETexture::Metal);
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

			std::shared_ptr<CActor> Actor = Scene->Create<CActor>(Spec, ETexture::White, FColor::LightGreen);
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

			std::shared_ptr<CActor> Actor = Scene->Create<CActor>(Spec, ETexture::Bricks);
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

			std::shared_ptr<CActor> Actor = Scene->Create<CActor>(Spec, ETexture::White, FColor::Convert(RGBA32::Magenta));
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

			std::shared_ptr<CActor> Actor = Scene->Create<CActor>(Spec, ETexture::White, FColor::Convert(RGBA32::DarkCyan));
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

			std::shared_ptr<CActor> Actor = Scene->Create<CActor>(Spec, ETexture::White, FColor::Gray);
		}

		CSpawner::CreateStaticPolygon(
			"Right-Wall-1",
			{ 4.830f, -0.90f }, /* Pos */
			{ 0.10f, 1.20f },   /* Size */
			FColor::Convert(RGBA32::Magenta)
		);
	}

	void CTestLevel::UI_Level()
	{
		ImGui::SetNextWindowBgAlpha(UI_BG_ALPHA);
		UI::PrepareRightSidebar();
		if (!ImGui::Begin(UI::PanelID::Sidebar2))
		{
			ImGui::End();
			return;
		}

		UI::Font::Push(EFont::SourceSansPro, EFontSize::Regular, EFontModifier::Normal);

		if (ImGui::TreeNodeEx("Info", ImGuiTreeNodeFlags_SpanAvailWidth))
		{
			ImGui::Text("Viewport: (%d, %d)", ViewportWidth, ViewportHeight);
			ImGui::Text("Editor Viewport: (%d, %d)", EditorViewportWidth, EditorViewportHeight);
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
			{
				ImGui::SeparatorText("Mouse");
				const glm::vec2 MouseViewportPos = GetMouseInViewportSpace();
				ImGui::Text("Viewport Space: (%.2f, %.2f)", MouseViewportPos.x, MouseViewportPos.y);
				if (CCamera* Camera = GetActiveCamera(); Camera != nullptr)
				{
					const glm::vec2 MouseWorldPos = GetMouseInWorldSpace(*Camera);
					ImGui::Text("World Space: (%.2f, %.2f)", MouseWorldPos.x, MouseWorldPos.y);
				}
				else
				{
					ImGui::Text("World Space: UNKNOWN");
				}
			}

			ImGui::Dummy(ImVec2(0, 8));
			{
				ImGui::Checkbox("Raycast Scene", &bRaycastScene);
				if (!bRaycastScene)
				{
					ImGui::BeginDisabled();
				}
				ImGui::Checkbox("Draw Debug Ray", &Config.Debug.bDrawRayHits);
				if (!bRaycastScene)
				{
					ImGui::EndDisabled();
				}
			}

			ImGui::Dummy(ImVec2(0, 12));
			ImGui::SeparatorText("Selection");
			{
				std::string Selected = "None";
				if (std::shared_ptr<CActor> Actor = SelectedActor.lock(); Actor != nullptr)
				{
					Selected = Actor->GetName();
				}
				ImGui::Text("Selected: %s", Selected.c_str());
			}

			ImGui::Dummy(ImVec2(0, 12));
			ImGui::SeparatorText("Serialization");
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

			ImGui::TreePop();
		}

		ImGui::Dummy(ImVec2(0, 8));

		/**
		 * Get vector copy of all actors to not invalidate the iteration
		 * if an actor is deleted.
		 */
		const auto Actors = Scene->GetActors();
		ImGui::Text("Actors: %d", Actors.size() + 1);
		UI::Draw::ActorNode(Player, Scene);
		for (auto& Actor : Actors)
		{
			UI::Draw::ActorNode(Actor, Scene);
		}

		ImGui::Dummy(ImVec2(0, 10));
		ImGui::Separator();
		ImGui::Dummy(ImVec2(0, 10));
		UI::CreatorMenu(Scene);

		UI::Font::Pop();
		ImGui::End();
	}

	void CTestLevel::UI_Player()
	{
		ImGui::SetNextWindowBgAlpha(UI_BG_ALPHA); /* @todo Dock node alpha. */
		UI::PrepareLeftSidebar();
		if (!ImGui::Begin(UI::PanelID::Sidebar1))
		{
			ImGui::End();
			return;
		}

		UI::PlayerData(Player);

		ImGui::End(); /* ~Player */
	}

	void CTestLevel::UI_ViewportTexture()
	{
		const ImVec2 WindowSize = {
			static_cast<float>(EditorViewportWidth),
			static_cast<float>(EditorViewportHeight)
		};

		std::shared_ptr<CFramebuffer> Framebuffer = CRenderer::GetViewportFramebuffer();
		std::shared_ptr<CTexture> ViewportTexture = Framebuffer->GetImage(0);

		ImGui::Image(
			(ImTextureID)ViewportTexture->GetID(),
			WindowSize,
			ImVec2(0, 1),       /* UV0 */
			ImVec2(1, 0),       /* UV1 */
			ImVec4(1, 1, 1, 1), /* Tint Color   */
			ImVec4(1, 1, 0, 1)  /* Border Color */
		);
	}

	void CTestLevel::DrawBackground() const
	{
		const CTexture& BgTexture = *CRenderer::GetTexture(ETexture::Background);
		const glm::vec2 HalfSize = GetActiveCamera()->GetHalfSize();
		const glm::vec2 BgSize(HalfSize.x * 3.0f, HalfSize.y * 4.0f);
		CRenderer::DrawQuad({ 0.0f, 0.0f, 0.0f }, BgSize, BgTexture, FColor::White);
	}

	void CTestLevel::OnWindowResized(const uint16_t InWidth, const uint16_t InHeight)
	{
		LK_TRACE_TAG("TestLevel", "Window resized: ({}, {})", ViewportWidth, ViewportHeight);
		ViewportWidth = InWidth;
		ViewportHeight = InHeight;
	}

	void CTestLevel::OnKeyPressed(const FKeyData& Data)
	{
		switch (Data.Key)
		{
			case EKey::Q:
				Gizmo = -1;
				break;

			case EKey::W:
				Gizmo = ImGuizmo::OPERATION::TRANSLATE;
				break;

			case EKey::E:
				Gizmo = ImGuizmo::OPERATION::ROTATE;
				break;

			case EKey::R:
				Gizmo = ImGuizmo::OPERATION::SCALE;
				break;
		}
	}

	void CTestLevel::OnMouseButtonPressed(const FMouseButtonData& Data)
	{
		switch (Data.State)
		{
			case EMouseButtonState::Pressed:
			{
				if (Data.Button == EMouseButton::Button0)
				{
					MousePickScene();
				}
				else if (Data.Button == EMouseButton::Button1)
				{
				}

				break;
			}
			case EMouseButtonState::Released:
			{
				break;
			}
			case EMouseButtonState::Held:
			{
				break;
			}
		}
	}

	void CTestLevel::OnSensorBeginEvent(const CSensorBeginEvent& Event)
	{
		LK_ASSERT(Event.Sensor && Event.Visitor);
		LK_DEBUG_TAG("TestLevel", "OnSensorBeginEvent: Sensor={} Visitor={}", Event.Sensor->GetName(), Event.Visitor->GetName());
	}

	void CTestLevel::OnSensorEndEvent(const CSensorEndEvent& Event)
	{
		LK_ASSERT(Event.Sensor && Event.Visitor);
		LK_DEBUG_TAG("TestLevel", "OnSensorEndEvent: Sensor={} Visitor={}", Event.Sensor->GetName(), Event.Visitor->GetName());
	}

	void CTestLevel::MousePickScene()
	{
		CCamera* Camera = GetActiveCamera();
		if (!Scene || !Camera)
		{
			return;
		}

		static std::vector<FHitResult> HitResults;
		const uint16_t Picked = PickSceneAtMouse(Scene, HitResults);
		if (Picked > 0)
		{
			const FHitResult& Hit = HitResults.at(0);
			if (std::shared_ptr<CActor> Ref = Hit.Ref.lock(); Ref != nullptr)
			{
				CSelectionContext::Select(Ref->GetHandle());
				SelectedActor = Ref;
			}
		}
	}

	void CTestLevel::RaycastScene()
	{
		CCamera* Camera = GetActiveCamera();
		if (!Scene || !Camera)
		{
			return;
		}

		static std::vector<FHitResult> HitResults;
		const uint16_t Hits = RaycastScene(Scene, HitResults);
#if 0 /* Disable for now since selection is overkill for raycasts */
		if (Hits > 0)
		{
			const FHitResult& Hit = HitResults.at(0);
			if (std::shared_ptr<CActor> Ref = Hit.Ref.lock(); Ref != nullptr)
			{
				SelectedActor = Ref;
			}
		}
#endif
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
				std::shared_ptr<CActor> Actor = Scene->Create<CActor>(ActorHandle, BodySpec, ActorTexture, ActorColor);
			}
			else
			{
				LK_ERROR_TAG("TestLevel", "Duplicate actors found during deserialization with handle {}", ActorHandle);
			}
		}
	}

	void CTestLevel::UI_PrepareEditorViewport()
	{
		ImGuiWindow* Window = ImGui::FindWindowByName(UI::PanelID::EditorViewport);
		if (!Window)
		{
			return;
		}

		ImGuiDockNode* DockNode = Window->DockNode;
		if (!DockNode)
		{
			return;
		}

		if ((DockNode->Size.x <= 0.0f) || (DockNode->Size.y <= 0.0f))
		{
			return;
		}

		ImGuiViewport* Viewport = ImGui::GetWindowViewport();
		ImGuiStyle& Style = ImGui::GetStyle();

		/* Modify the size on the y-axis to account for the docking separators. */
		DockNode->Size = ImVec2(DockNode->Size.x, (DockNode->Size.y - Style.DockingSeparatorSize));

		Window->Flags |= ImGuiWindowFlags_NoTitleBar;
		DockNode->LocalFlags |= ImGuiDockNodeFlags_NoWindowMenuButton | ImGuiDockNodeFlags_NoTabBar;
		DockNode->LocalFlags &= ~ImGuiDockNodeFlags_NoDocking;
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

		const CActor* ActorA = static_cast<CActor*>(b2Shape_GetUserData(ShapeA));
		const CActor* ActorB = static_cast<CActor*>(b2Shape_GetUserData(ShapeB));

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
