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
#include "renderer/ui/editor_resources.h"
#include "renderer/ui/ui.h"
#include "renderer/ui/widgets.h"
#include "renderer/ui/selectionpanel.h"
#include "physics/body.h"
#include "physics/physicsworld.h"
#include "physics/ray.h"
#include "serialization/serialization.h"

namespace platformer2d::Level {

	namespace {
		/*************************************
		 *        GAME SPECIFICATION
		 *************************************/
		const FGameSpecification GameSpec = {
			.LevelFilepath = std::filesystem::path(LEVELS_DIR "/testlevel.yaml"),
			.Name = "TestLevel",
			.Gravity = { 0.0f, -5.0f },
			.Zoom = 0.32f,

			.Player = {
				.ActorSpec = FActorSpecification(ETexture::Player),
				.BodySpec = {
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
			}
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
	}

	static bool PreSolve(b2ShapeId ShapeA, b2ShapeId ShapeB, b2Vec2 Point, b2Vec2 Normal, void* Ctx);

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
			UI::Widget::OnActorDeleted(Handle);
		});

		const FGameSpecification& Spec = GetSpecification();
		CPhysicsWorld::SetGravity(Spec.Gravity);
		CPhysicsWorld::OnSensorBeginEvent.Add(this, &CTestLevel::OnSensorBeginEvent);
		CPhysicsWorld::OnSensorEndEvent.Add(this, &CTestLevel::OnSensorEndEvent);

		CreatePlayer();
		LK_VERIFY(Player);

		Deserialize(GameSpec.LevelFilepath);

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

		LK_DEBUG_TAG("TestLevel", "Initialize editor resources");
		EditorResources.Initialize();
	}

	void CTestLevel::Destroy()
	{
		LK_TRACE_TAG("TestLevel", "Destroy");
		Serialize(GameSpec.LevelFilepath);

		LK_DEBUG_TAG("TestLevel", "Release level resources");
		Player.reset();
		Scene.reset();

		EditorResources.Destroy();
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
		Camera.SetViewportSize(EditorViewportWidth, EditorViewportHeight);
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
			glm::degrees(Player->GetRotation()),
			Player->GetOutlineThickness(),
			Player->GetOutlineColor()
		);

		Scene->Render();
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

		UI_PrepareEditorViewport();
		UI::Begin(UI::PanelID::EditorViewport, nullptr, UI::EditorViewportFlags);
		{
			ImGui::PopStyleVar(2); /* FramePadding, WindowPadding */
			UpdateEditorViewportBounds();
			UI_ViewportTexture();

			UI_Level();
			UI_Player();

			UI::Statistics();
			UI::SelectionPanel();
			UI_DrawGizmo();
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
		const YAML::Node& SceneNode = Data["Scene"];
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
		FActorSpecification ActorSpec;
		ActorSpec.Texture = ETexture::Player;
		Player = std::make_shared<CPlayer>(Spec.Player.ActorSpec , Spec.Player.BodySpec);
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
		UI::Widget::ActorNode(Player, Scene);
		for (auto& Actor : Actors)
		{
			UI::Widget::ActorNode(Actor, Scene);
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
			ImVec4(1, 1, 1, 0)  /* Border Color */
		);
	}

	void CTestLevel::UI_DrawGizmo()
	{
		std::shared_ptr<CActor> SelectedRef = SelectedActor.lock();
		if (!SelectedRef)
		{
			return;
		}

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

		if (!Player || (Event.Sensor != Player.get()) && (Event.Visitor != Player.get()))
		{
			return;
		}

		/**
		 * Player is overlapping the sensor.
		 * Determine the type of sensor.
		 */
		if (Event.Visitor == Player.get())
		{
			if (auto* IC = Event.Sensor->TryGetComponent<FInteractionComponent>())
			{
				LK_WARN("IC: {}", Enum::ToString(IC->GetType()));
			}
		}
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
