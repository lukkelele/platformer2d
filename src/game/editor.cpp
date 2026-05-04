#include "editor.h"

#include <fstream>
#include <istream>
#include <numeric>

#include "core/profiler.h"
#include "core/timer.h"
#include "core/window.h"
#include "core/selectioncontext.h"
#include "core/string.h"
#include "core/input/keyboard.h"
#include "core/input/mouse.h"
#include "core/math/math.h"
#include "game/gameplaysystem.h"
#include "game/enemy.h"
#include "game/player.h"
#include "game/spawner.h"
#include "game/controller/patrolcontroller.h"
#include "renderer/renderer.h"
#include "renderer/debugrenderer.h"
#include "renderer/ui/editor_resources.h"
#include "renderer/ui/pausemenu.h"
#include "renderer/ui/selectionpanel.h"
#include "renderer/ui/ui.h"
#include "renderer/ui/widgets.h"
#include "physics/body.h"
#include "physics/physicsworld.h"
#include "physics/ray.h"
#include "serialization/serialization.h"

namespace platformer2d {

	namespace {
		/*************************************
		 *        GAME SPECIFICATION
		 *************************************/
		const FGameSpecification GameSpec = {
			/* clang-format off */
			.InstanceName = "Editor",
			.LevelFilepath = std::filesystem::path(LEVELS_DIR "/editor.yaml"),
			.Player = {
				.ActorSpec = FActorSpecification(ETexture::Player),
				.BodySpec = {
					.Type = EBodyType::Dynamic,
					.Shape = FCapsule{
						.P0 = { 0.0f, -0.02f },
						.P1 = { 0.0f,  0.02f },
						.Radius = 0.10f,
					},
					.Position = { 0.0f, 0.50f },
					.Friction = 0.620f,
					.Density = 0.60f,
					.LinearDamping = 0.560f,
					.Flags = EBodyFlag::EBodyFlag_SensorEvents,
					.MotionLock = EMotionLock_Z,
				},
			}
			/* clang-format on */
		};

		constexpr float UI_BG_ALPHA = 0.70f;
		constexpr const char* UI_ID_LEVEL = "Level";
		constexpr const char* UI_ID_PLAYER = "Player";

		std::weak_ptr<CActor> SelectedActor;
		std::weak_ptr<CActor> RotatingPlatform;

		const std::array<const char*, CRenderer::MaxTextures> TextureNames = {
			Enum::ToString(ETexture::White),
			Enum::ToString(ETexture::Background),
			Enum::ToString(ETexture::Player),
			Enum::ToString(ETexture::Metal),
			Enum::ToString(ETexture::Bricks),
			Enum::ToString(ETexture::Wood),
		};

		int Gizmo = ImGuizmo::OPERATION::TRANSLATE;
		bool bRaycastScene = false;
		bool bSceneStateChanged = false;
		glm::vec2 PLAYER_SPAWN = {0.0f, 0.0f};
		glm::vec2 GRAVITY = {0.0f, -5.0f};
		glm::vec2 GRAVITY_CACHED = GRAVITY; /* Updated during scene termination. */
		float SCENE_LOAD_CAMERA_ZOOM = 0.30f;

		/* @todo Remove from here. Just temporary */
		bool bDrawCircle = false;
		bool bDrawCircleFilled = false;
		bool bDrawLine = false;
		glm::vec3 P1 = {0.30f, -0.40, 0.50f};
		glm::vec3 DebugRot = {0.0f, 0.0f, 0.0f};
		float DebugRadius = 0.05f;
	}

	static bool PreSolve(b2ShapeId ShapeA, b2ShapeId ShapeB, b2Vec2 Point, b2Vec2 Normal, void* Ctx);

	static void UpdateInputBuffer(const std::size_t Count)
	{
		std::snprintf(UI::ActorAttr.NameBuf.data(), sizeof(UI::ActorAttr.NameBuf), "Actor-%lld", Count + 2);
	}

	CEditor::CEditor()
		: CGameInstance(this, GameSpec)
	{
		LK_TRACE_TAG("Editor", "Instance created");
		CRenderer::SetClearColor(FColor::SkyBlue);
	}

	CEditor::~CEditor()
	{
		LK_TRACE_TAG("Editor", "Destructor");
	}

	void CEditor::Initialize()
	{
		LK_DEBUG_TAG("Editor", "Initialize");
		LK_VERIFY(Player == nullptr);

		DelegateHandles.OnSensorBeginEvent = CPhysicsWorld::OnSensorBeginEvent.Add(this, &CEditor::OnSensorBeginEvent);
		DelegateHandles.OnSensorEndEvent = CPhysicsWorld::OnSensorEndEvent.Add(this, &CEditor::OnSensorEndEvent);
		DelegateHandles.OnContactBeginEvent = CPhysicsWorld::OnContactBeginEvent.Add(this, &CEditor::OnContactBeginEvent);
		DelegateHandles.OnContactEndEvent = CPhysicsWorld::OnContactEndEvent.Add(this, &CEditor::OnContactEndEvent);

		Deserialize(GameSpec.LevelFilepath);
		OpenScene(SceneToOpen);
		LastSceneFilepath = Scene->GetFilepath();
		LK_DEBUG_TAG("Editor", "Last scene filepath: {}", LastSceneFilepath);

		CWindow::OnResized.Add(this, &CEditor::OnWindowResized);
		CWindow* Window = CWindow::Get();
		Window->Maximize();
		UpdateViewportBounds();

		DelegateHandles.OnKeyPressed = CKeyboard::OnKeyPressed.Add(this, &CEditor::OnKeyPressed);
		DelegateHandles.OnMouseButtonPressed = CMouse::OnButtonPressed.Add(this, &CEditor::OnMouseButtonPressed);

		LK_DEBUG_TAG("Editor", "Initialize editor resources");
		EditorResources.Initialize();

		DelegateHandles.OnActorCreated = CScene::OnActorCreated.Add([&](const LUUID Handle, std::weak_ptr<CActor> ActorRef)
		{
			if (!Scene) {
				return;
			}
			if (std::shared_ptr<CActor> Actor = ActorRef.lock(); Actor != nullptr) {
				LK_TRACE_TAG("Editor", "OnActorCreated: {} ({})", Actor->GetName(), Handle);
				LK_ASSERT(Scene);
				UpdateInputBuffer(Scene->GetActors().size());
			}
		});

		DelegateHandles.OnActorDeleted = CScene::OnActorDeleted.Add([&](const LUUID Handle)
		{
			if (!Scene) {
				return;
			}
			LK_DEBUG_TAG("Editor", "OnActorDeleted: {}", Handle);
			UpdateInputBuffer(Scene->GetActors().size());
			UI::Widget::OnActorDeleted(Handle);
		});

		DelegateHandles.OnPauseMenuOpened = UI::OnPauseMenuOpened.Add([&](const bool Opened)
		{
			if (!Scene) {
				LK_TRACE_TAG("Editor", "Pause menu toggled, no scene active");
				return;
			}

			if (Opened) {
				PauseGame();
			} else {
				ResumeGame();
			}
		});

		/**
		 * Target player 0 by default.
		 * Has to be done here because the scene serialization does not
		 * guarantee the player instance is valid by the time the controller instance
		 * is constructed and OnPossess is invoked.
		 *
		 * In the future a better approach would be to ensure player instances are
		 * created first.
		 */
		const auto EnemyActors = Scene->GetAllOfType<CEnemy>();
		for (const auto& EnemyActor : EnemyActors) {
			IEnemyController* Controller = EnemyActor->GetController();
			if (!Controller) {
				continue;
			}

			const EControllerType ControllerType = Controller->GetControllerType();
			if (ControllerType == EControllerType::Patrol) {
				static_cast<CPatrolController*>(Controller)->SetTarget(GetPlayer(0));
			}
		}
	}

	void CEditor::Destroy()
	{
		LK_DEBUG_TAG("Editor", "Destroy");
		/* Release bound delegates. */
		CWindow::OnResized.Remove(DelegateHandles.OnWindowResized);
		CKeyboard::OnKeyPressed.Remove(DelegateHandles.OnKeyPressed);
		CMouse::OnButtonPressed.Remove(DelegateHandles.OnMouseButtonPressed);
		CPhysicsWorld::OnSensorBeginEvent.Remove(DelegateHandles.OnSensorBeginEvent);
		CPhysicsWorld::OnSensorEndEvent.Remove(DelegateHandles.OnSensorEndEvent);
		CPhysicsWorld::OnContactBeginEvent.Remove(DelegateHandles.OnContactBeginEvent);
		CPhysicsWorld::OnContactEndEvent.Remove(DelegateHandles.OnContactEndEvent);
		CScene::OnActorCreated.Remove(DelegateHandles.OnActorCreated);
		CScene::OnActorDeleted.Remove(DelegateHandles.OnActorDeleted);
		UI::OnPauseMenuOpened.Remove(DelegateHandles.OnPauseMenuOpened);

		Serialize(GameSpec.LevelFilepath);
		CloseScene();

		LK_DEBUG_TAG("Editor", "Release level resources");
		Player.reset();
		Player = nullptr;
		Scene.reset();
		Scene = nullptr;

		EditorResources.Destroy();
		LK_VERIFY(!CPhysicsWorld::IsValid(), "Physics world still active");
	}

	void CEditor::OnAttach()
	{
		LK_DEBUG_TAG("Editor", "OnAttach");
		Initialize();
	}

	void CEditor::OnDetach()
	{
		LK_DEBUG_TAG("Editor", "OnDetach");
		Destroy();
	}

	void CEditor::Tick(const float InDeltaTime)
	{
		LK_PROFILE_FUNC();
		const ESceneState SceneState = Scene ? Scene->GetState() : ESceneState::None;
		if (SceneState == ESceneState::Play) {
			DeltaTime = InDeltaTime;
		} else {
			/* Freeze the scene. */
			DeltaTime = 0.0f;
		}

		if (!Scene) {
			if (bOpenSceneNextTick) {
				OpenScene(SceneToOpen);
				bOpenSceneNextTick = false;
			}
			return;
		} else if (bCloseSceneNextTick) {
			CloseScene();
			bCloseSceneNextTick = false;
			return;
		}

		if (bSceneStateChanged) {
			if (SceneState == ESceneState::Play) {
				CPhysicsWorld::Unpause();
			} else {
				CPhysicsWorld::Pause();
			}
			bSceneStateChanged = false;
		}

		CCamera& Camera = Player->GetCamera();
		Camera.SetViewportSize(EditorViewportWidth, EditorViewportHeight);
		CRenderer::BeginScene(Camera);

		if (std::shared_ptr<CActor> Selected = Scene->GetActor(CSelectionContext::GetSelected())) {
			SelectedActor = Selected;
		}

		Player->Tick(DeltaTime);
		Scene->Tick(DeltaTime);

		if (bRaycastScene) {
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
			Player->GetOutlineColor());

		/* @fixme Just temporarily here */
		if (bDrawCircle) {
			CRenderer::DrawCircle(P1, DebugRot, DebugRadius, FColor::Red);
		}
		if (bDrawCircleFilled) {
			CRenderer::DrawCircleFilled(P1, DebugRadius, FColor::Red, 5.0f);
		}
		if (bDrawLine) {
			CRenderer::DrawLine(glm::vec3(0.0f, 0.0f, 1.0f), P1, FColor::Black, 6);
		}

		UI::RenderChainPreview(Scene);

		Scene->Render();
	}

	void CEditor::RenderUI()
	{
		LK_PROFILE_FUNC();
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		if (!UI::Begin(UI::PanelID::CoreViewport, nullptr, UI::CoreViewportFlags)) {
			ImGui::PopStyleVar(2);
			return;
		}

		UI::PrepareViewport();
		const bool EditorViewportOpen = UI::Begin(UI::PanelID::Viewport, nullptr, UI::ViewportFlags);
		ImGui::PopStyleVar(2);
		if (EditorViewportOpen) {
			UpdateEditorViewportState();
			if (Scene) {
				UI_ViewportTexture();
			} else {
				UI::LevelLauncher();
			}

			UI_Level();

			if (Scene) {
				UI::Statistics();
				UI::PlayerHud(Player);
				UI::SelectionPanel();
				UI_DrawGizmo();
			}

			UI::End(); /* ~Viewport */
		}

		UI::PrepareMenuBar();
		if (UI::Begin(UI::PanelID::Menubar, nullptr, UI::SidebarFlags | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar)) {
			UI_MainMenubar();
			UI::End();
		}

		UI::PrepareTopBar();
		if (UI::Begin(UI::PanelID::Topbar, nullptr, UI::SidebarFlags | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar)) {
			UI_Topbar();
			UI::End();
		}

		UI_LeftSidebar();

		UI::End(); /* ~CoreViewport */
	}

	CCamera* CEditor::GetActiveCamera() const
	{
		return (Player ? &Player->GetCamera() : nullptr);
	}

	std::shared_ptr<CPlayer> CEditor::GetPlayer(const std::size_t Idx) const
	{
		LK_ASSERT(Idx == 0, "Only 1 player supported");
		return Player;
	}

	void CEditor::PauseGame()
	{
		LK_DEBUG_TAG("Editor", "Game paused");
		CPhysicsWorld::Pause();
		if (Scene) {
			Scene->SetState(ESceneState::Pause);
		}
	}

	void CEditor::ResumeGame()
	{
		LK_DEBUG_TAG("Editor", "Game resumed");
		CPhysicsWorld::Unpause();
		if (Scene) {
			Scene->SetState(ESceneState::Play);
		}
	}

	bool CEditor::IsGamePaused()
	{
		return (Scene ? (Scene->GetState() == ESceneState::Pause) : true);
	}

	uint16_t CEditor::RaycastScene(std::shared_ptr<CScene> TargetScene, std::vector<FHitResult>& HitResults)
	{
		static FRayCast RayData;
		HitResults.clear();

		const glm::vec2 MousePos = GetMouseInViewportSpace();
		if ((MousePos.x < -1.0f) || (MousePos.x > 1.0f) || (MousePos.y < -1.0f) || (MousePos.y > 1.0f)) {
			return 0;
		}

		const CCamera& Camera = *GetActiveCamera();
		Physics::CastRay(
			RayData,
			Camera.GetPosition(),
			Camera.GetViewMatrix(),
			Camera.GetProjectionMatrix(),
			MousePos.x,
			MousePos.y);

		for (const auto& Actor : TargetScene->GetActors()) {
			const glm::vec2 Pos = Actor->GetPosition();
			const glm::vec2 Size = Actor->GetSize();
			const glm::vec2 HalfSize = Size * 0.50f;
			const glm::vec2 BoxMin = Pos - HalfSize;
			const glm::vec2 BoxMax = Pos + HalfSize;

			float T = 0.0f;
			if (Physics::RaycastAABB(RayData, BoxMin, BoxMax, T)) {
				HitResults.push_back(FHitResult{Actor->GetHandle(), Actor, T});

				if (Config.Debug.bDrawRayHits) {
					CDebugRenderer::DrawRayHit(RayData, T);
				}
			}
		}

		if (HitResults.empty()) {
			return 0;
		}

		std::sort(HitResults.begin(), HitResults.end(), [](auto& Lhs, auto& Rhs)
		{
			return Lhs.Distance < Rhs.Distance;
		});
		return static_cast<uint16_t>(HitResults.size());
	}

	uint16_t CEditor::PickSceneAtMouse(std::shared_ptr<CScene> TargetScene, std::vector<FHitResult>& HitResults)
	{
		HitResults.clear();
		const CCamera& Camera = *GetActiveCamera();
		const glm::vec2 MouseWorld = GetMouseInWorldSpace(Camera);
		if (!std::isfinite(MouseWorld.x) || !std::isfinite(MouseWorld.y)) {
			return 0;
		}

		for (const auto& Actor : TargetScene->GetActors()) {
			const glm::vec2 Pos = Actor->GetPosition();
			const glm::vec2 Size = Actor->GetSize();
			const float Rotation = Actor->GetRotation();
			if (Math::IsPointInPolygon(MouseWorld, Pos, Size, Rotation)) {
				FHitResult Entry{};
				Entry.Handle = Actor->GetHandle();
				Entry.Ref = Actor;

				const glm::vec2 Delta = MouseWorld - Pos;
				Entry.Distance = glm::length(Delta);

				HitResults.push_back(Entry);
			}
		}

		if (HitResults.empty()) {
			return 0;
		}

		std::sort(HitResults.begin(), HitResults.end(), [](const auto& Lhs, const auto& Rhs)
		{
			return Lhs.Distance < Rhs.Distance;
		});
		return static_cast<uint16_t>(HitResults.size());
	}

	void CEditor::OnSensorBeginEvent(const CSensorBeginEvent& Event)
	{
		LK_ASSERT(Event.Sensor && Event.Visitor);
		LK_DEBUG_TAG("Editor", "OnSensorBeginEvent: Sensor={} Visitor={}", Event.Sensor->GetName(), Event.Visitor->GetName());
		if (!Player || (Event.Sensor != Player.get()) && (Event.Visitor != Player.get())) {
			return;
		}

		/**
		 * Player is overlapping the sensor.
		 * Determine the type of sensor.
		 */
		if (Event.Visitor == Player.get()) {
			if (auto* IC = Event.Sensor->TryGetComponent<FInteractionComponent>()) {
				LK_DEBUG("[BEGIN] Interaction: {}", Enum::ToString(IC->GetType()));
				Event.Sensor->SetOutlineEnabled(true);

				switch (IC->GetType()) {
					case EInteraction::Damage:
					{
						auto& Data = std::get<FDamageInteraction>(IC->GetData());
						LK_WARN("Damage={}", Data.Damage);
						break;
					}
					case EInteraction::Pickup:
					{
						CPlayer& PlayerRef = *static_cast<CPlayer*>(Event.Visitor);
						OnPickupEvent(PlayerRef, *IC);
						break;
					}
					default:
						break;
				}
			}
		}
	}

	void CEditor::OnSensorEndEvent(const CSensorEndEvent& Event)
	{
		LK_ASSERT(Event.Sensor && Event.Visitor);
		LK_DEBUG_TAG("Editor", "OnSensorEndEvent: Sensor={} Visitor={}", Event.Sensor->GetName(), Event.Visitor->GetName());
		if (!Player || (Event.Sensor != Player.get()) && (Event.Visitor != Player.get())) {
			return;
		}

		/**
		 * Player is overlapping the sensor.
		 * Determine the type of sensor.
		 */
		if (Event.Visitor == Player.get()) {
			if (auto* IC = Event.Sensor->TryGetComponent<FInteractionComponent>()) {
				LK_DEBUG("[END] Interaction: {}", Enum::ToString(IC->GetType()));
				Event.Sensor->SetOutlineEnabled(false);
			}
		}
	}

	static void OnProjectileContact(CActor* ProjectileActor, CActor* HitActor)
	{
		CProjectile* Projectile = static_cast<CProjectile*>(ProjectileActor);
		/* Do not destroy if the actor hit is the player who shot the projectile. */
		if (Projectile->GetOwner() && Projectile->GetOwner()->IsHeldBy(HitActor)) {
			return;
		}

		Projectile->BounceCount++;

		LK_ASSERT(HitActor, "Invalid rojectile hit");
		LK_TRACE("{}: Hit: {} ({})", ProjectileActor->GetName(), HitActor->GetName(), Enum::ToString(HitActor->GetActorType()));

		if (HitActor->HasComponent<FHealthComponent>()) {
			auto& HC = HitActor->GetComponent<FHealthComponent>();
			if (HC.IsDamageable()) {
				if (HitActor->GetActorType() == EActorType::Enemy) {
					CEnemy& EnemyRef = HitActor->As<CEnemy>();
					const float Damage = Projectile->GetDamage();
					HC.SetHealth(HC.GetHealth() - Damage);
					if (HC.IsDead()) {
						LK_INFO_TAG("Editor", "Killed {}", HitActor->GetName());
						EnemyRef.Kill();
					}
				}
			} else {
				LK_DEBUG_TAG("Editor", "Hit actor {} has health component but is not damageable", HitActor->GetName());
			}
		}

		if (Projectile->ExplodesOnImpact()) {
			Projectile->Destroy();
		} else if (Projectile->BounceCount >= Projectile->MaxBounceCount) {
			LK_TRACE("{}: Max bounce reached: {}", Projectile->GetName(), Projectile->BounceCount);
			Projectile->Destroy();
		}
	};

	void CEditor::OnContactBeginEvent(const CContactBeginEvent& Event)
	{
		LK_TRACE_TAG("Editor", "OnContactBeginEvent: A={} B={}", (Event.A ? Event.A->GetName() : "NULL"), (Event.B ? Event.B->GetName() : "NULL"));
		LK_ASSERT(Event.A && Event.B, "Invalid event references");
		if (!Event.A || !Event.B) {
			return;
		}

		const EActorType AType = Event.A->GetActorType();
		const EActorType BType = Event.B->GetActorType();

		/* Projectile hit. */
		if (AType == EActorType::Projectile) {
			OnProjectileContact(Event.A, Event.B);
		} else if (BType == EActorType::Projectile) {
			OnProjectileContact(Event.B, Event.A);
		}
	}

	void CEditor::OnContactEndEvent(const CContactEndEvent& Event)
	{
		LK_TRACE_TAG("Editor", "OnContactEndEvent: A={} B={}", (Event.A ? Event.A->GetName() : "NULL"), (Event.B ? Event.B->GetName() : "NULL"));
		LK_ASSERT(Event.A && Event.B, "Invalid event references");
		if (!Event.A || !Event.B) {
			return;
		}
	}

	bool CEditor::Serialize(const std::filesystem::path& OutFile) const
	{
		LK_INFO_TAG("Editor", "Serialize: {}", OutFile);
		YAML::Emitter Out;

		Out << YAML::BeginMap; /* Level */
		Out << YAML::Key << "Level" << YAML::Value << LayerName;

		LK_TRACE("LastSceneFilepath: {}", LastSceneFilepath);
		const std::filesystem::path ScenePath = (Scene ? Scene->GetFilepath() : LastSceneFilepath);
		LK_DEBUG_TAG("Editor", "Scene path: {}", ScenePath);
		Out << YAML::Key << "Scene" << YAML::Value << Core::GetRelativeFromProject(ScenePath);

		Out << YAML::Key << "PlayerSpawn" << YAML::Value << PLAYER_SPAWN;
		Out << YAML::Key << "CameraZoom" << YAML::Value << SCENE_LOAD_CAMERA_ZOOM;

		/* Physics */
		Out << YAML::Key << "Physics";
		Out << YAML::BeginMap;
		Out << YAML::Key << "Gravity" << YAML::Value << (Scene ? CPhysicsWorld::GetGravity() : GRAVITY_CACHED);
		Out << YAML::EndMap;
		/* ~ Physics */

		Out << YAML::EndMap; /* ~Level */

		/* Save scene to its own file. */
		if (Scene) {
			Scene->Serialize(ScenePath);
		}

		std::ofstream File(OutFile);
		File << Out.c_str();

		return true;
	}

	bool CEditor::Deserialize(const std::filesystem::path& Filepath)
	{
		LK_INFO_TAG("Editor", "Deserialize: {}", StringUtils::GetPathRelativeToProject(Filepath));
		LK_ASSERT(std::filesystem::exists(Filepath), "Filepath does not exist: {}", Filepath);
		if (!std::filesystem::exists(Filepath)) {
			LK_ERROR_TAG("Editor", "Filepath does not exist: {}", Filepath);
			return false;
		}

		std::ifstream InputStream(Filepath);
		std::stringstream StringStream;
		StringStream << InputStream.rdbuf();
		const std::string YamlString = StringStream.str();

		const YAML::Node Data = YAML::Load(YamlString);

		LK_DESERIALIZE_PROPERTY(Gravity, GRAVITY, glm::vec2(0.0f, -5.0f), Data);
		LK_DESERIALIZE_PROPERTY(PlayerSpawn, PLAYER_SPAWN, glm::vec2(0.0f, 0.0f), Data);
		LK_DESERIALIZE_PROPERTY(CameraZoom, SCENE_LOAD_CAMERA_ZOOM, 0.40f, Data);

		/* Load the scene. */
		const YAML::Node& SceneNode = Data["Scene"];
		LK_ASSERT(!SceneNode.IsNull());
		if (SceneNode.IsNull()) {
			LK_ERROR_TAG("Editor", "Scene node is missing in YAML");
			return false;
		}

		const std::filesystem::path SceneFilepath = SceneNode.as<std::filesystem::path>();
		LK_INFO_TAG("Editor", "Scene to open: {}", StringUtils::GetPathRelativeToProject(SceneFilepath));
		SceneToOpen = SceneFilepath;

		return !SceneToOpen.empty();
	}

	void CEditor::UpdateEditorViewportState()
	{
		UpdateEditorViewportBounds();

		bEditorViewportFocused = ImGui::IsWindowFocused();
		bEditorViewportHovered = ImGui::IsWindowHovered();

		const auto [PosX, PosY] = CMouse::GetPos();
		bEditorViewportHovered = (PosX >= EditorViewportBounds[0].x) && (PosY >= EditorViewportBounds[0].y) && (PosX <= EditorViewportBounds[1].x) && (PosY <= EditorViewportBounds[1].y);
	}

	void CEditor::UpdateEditorViewportBounds()
	{
		ImGuiWindow* Window = ImGui::GetCurrentWindow();
		const ImVec2 WindowPos = Window->Pos;

		const ImVec2 RegionMin = ImGui::GetWindowContentRegionMin();
		const ImVec2 RegionMax = ImGui::GetWindowContentRegionMax();

		EditorViewportBounds[0] = {
			WindowPos.x + RegionMin.x,
			WindowPos.y + RegionMin.y};

		EditorViewportBounds[1] = {
			WindowPos.x + RegionMax.x,
			WindowPos.y + RegionMax.y};

		const float VpWidth = EditorViewportBounds[1].x - EditorViewportBounds[0].x;
		const float VpHeight = EditorViewportBounds[1].y - EditorViewportBounds[0].y;

		if ((EditorViewportWidth != static_cast<uint16_t>(VpWidth)) || (EditorViewportHeight != static_cast<uint16_t>(VpHeight))) {
			EditorViewportWidth = static_cast<uint16_t>(VpWidth);
			EditorViewportHeight = static_cast<uint16_t>(VpHeight);
			CRenderer::GetViewportFramebuffer()->Resize(EditorViewportWidth, EditorViewportHeight);
		}
	}

	void CEditor::UpdateViewportBounds()
	{
		ViewportBounds[0] = {0.0f, 0.0f};
		if (CWindow* Window = CWindow::Get(); Window != nullptr) {
			ViewportBounds[1] = Window->GetSize();
		} else {
			LK_WARN_TAG("Editor", "Failed to update viewport bounds");
			ViewportBounds[1] = {0.0f, 0.0f};
		}
	}

	glm::vec2 CEditor::GetMouseInViewportSpace()
	{
		auto [MouseX, MouseY] = CMouse::GetPos();
		MouseX -= EditorViewportBounds[0].x;
		MouseY -= EditorViewportBounds[0].y;
		const float VpWidth = EditorViewportBounds[1].x - EditorViewportBounds[0].x;
		const float VpHeight = EditorViewportBounds[1].y - EditorViewportBounds[0].y;

		return glm::vec2(
			(MouseX / static_cast<float>(VpWidth)) * 2.0f - 1.0f,
			((MouseY / static_cast<float>(VpHeight)) * 2.0f - 1.0f) * -1.0f);
	}

	glm::vec2 CEditor::GetMouseInWorldSpace(const CCamera& Camera)
	{
		const glm::vec2 MousePos = GetMouseInViewportSpace();
		if ((MousePos.x < -1.0f) || (MousePos.x > 1.0f) || (MousePos.y < -1.0f) || (MousePos.y > 1.0f)) {
			return glm::vec2(std::numeric_limits<float>::quiet_NaN());
		}

		const glm::vec4 ClipPos = glm::vec4(MousePos.x, MousePos.y, 0.0f, 1.0f);
		const glm::mat4 InvViewProj = glm::inverse(Camera.GetProjectionMatrix() * Camera.GetViewMatrix());
		glm::vec4 WorldPos = InvViewProj * ClipPos;
		if (WorldPos.w != 0.0f) {
			WorldPos /= WorldPos.w;
		}

		return WorldPos;
	}

	void CEditor::CreatePlayer()
	{
		const FGameSpecification& Spec = GetSpecification();
		FActorSpecification ActorSpec;
		ActorSpec.Texture = ETexture::Player;
		Player = std::make_shared<CPlayer>(Spec.Player.ActorSpec, Spec.Player.BodySpec);

		Player->OnJumped.Add([](const FPlayerData& PlayerData)
		{
			LK_TRACE("Player {} jumped", PlayerData.ID);
		});

		Player->OnLanded.Add([](const FPlayerData& PlayerData)
		{
			LK_TRACE("Player {} landed", PlayerData.ID);
		});

		CGameplaySystem::Teleport(Player, PLAYER_SPAWN);
	}

	void CEditor::UI_Level()
	{
		ImGui::SetNextWindowBgAlpha(UI_BG_ALPHA);
		UI::PrepareRightSidebar();
		if (!UI::Begin(UI::PanelID::Sidebar2)) {
			return;
		}

		UI::Widget::SceneManagerPanel(Scene);
		UI::CreatorMenu(Scene);
		UI::ChainCreatorWidget(Scene);

		UI::Font::Push(EFont::SourceSansPro, EFontSize::Regular, EFontModifier::Normal);

		ImGui::Spacing();
		{
			if (ImGui::Button("Spawn Enemy")) {
				FEnemySpecification EnemySpec;

				FActorSpecification ActorSpec;
				auto Enemies = Scene->GetAllOfType<CEnemy>();
				ActorSpec.Name = Format("Enemy-{}", Enemies.size() + 1);
				ActorSpec.Texture = ETexture::Enemy1;

				FBodySpecification BodySpec;
				BodySpec.Type = EBodyType::Dynamic;
				BodySpec.Position = glm::vec2(0.0f, 0.0f);
				BodySpec.Flags = EBodyFlag_PreSolveEvents;
				FPolygon Polygon = {
					.Size = glm::vec2(0.20f, 0.20f),
				};
				BodySpec.Shape.emplace<FPolygon>(Polygon);

				if (Scene) {
					LK_INFO_TAG("Editor", "Creating enemy: {}", ActorSpec.Name);
					std::shared_ptr<CEnemy> Enemy = Scene->Create<CEnemy>(EnemySpec, ActorSpec, BodySpec);
					Enemy->SetController(std::make_unique<CPatrolController>(1.0f, 1.0f));
					Enemy->AddComponent<FHealthComponent>();
				}
			}

			UI::EnemiesInfo(Scene);
		}

		if (Player) {
			ImGui::Spacing();
			UI::Widget::Rifle(Player->GetRifle());
		}

		ImGui::Spacing();

		if (ImGui::TreeNodeEx("Info", ImGuiTreeNodeFlags_SpanAvailWidth)) {
			ImGui::Text("Viewport: (%d, %d)", ViewportWidth, ViewportHeight);
			ImGui::Text("Editor Viewport: (%d, %d)", EditorViewportWidth, EditorViewportHeight);
			{
				ImGuiViewport* Viewport = ImGui::GetMainViewport();
				ImGui::Text("Main Viewport: (%.1f, %.1f)", Viewport->Size.x, Viewport->Size.y);
			}

			const int Gcd = std::gcd(ViewportWidth, ViewportHeight);
			ImGui::Text("Aspect Ratio: %d/%d", (ViewportWidth / Gcd), (ViewportHeight / Gcd));

			if (CCamera* Camera = GetActiveCamera()) {
				const glm::vec2 HalfSize = Camera->GetHalfSize();
				ImGui::Text("Half Size: (%2.f, %.2f)", HalfSize.x, HalfSize.y);
			}

			if (CPhysicsWorld::IsValid()) {
				const b2Vec2 G = b2World_GetGravity(CPhysicsWorld::GetID());
				ImGui::Text("Gravity: (%.1f, %.1f)", G.x, G.y);
			} else {
				ImGui::Text("Gravity: No world");
			}

			ImGui::Dummy(ImVec2(0, 8));

			glm::vec4 ClearColor = CRenderer::GetClearColor();
			if (ImGui::SliderFloat3("Background", &ClearColor.x, 0.0f, 1.0f, "%.2f")) {
				CRenderer::SetClearColor(ClearColor);
			}

			ImGui::Dummy(ImVec2(0, 8));
			{
				ImGui::SeparatorText("Mouse");
				const glm::vec2 MouseViewportPos = GetMouseInViewportSpace();
				ImGui::Text("Viewport Space: (%.2f, %.2f)", MouseViewportPos.x, MouseViewportPos.y);
				if (CCamera* Camera = GetActiveCamera(); Camera != nullptr) {
					const glm::vec2 MouseWorldPos = GetMouseInWorldSpace(*Camera);
					ImGui::Text("World Space: (%.2f, %.2f)", MouseWorldPos.x, MouseWorldPos.y);
				} else {
					ImGui::Text("World Space: UNKNOWN");
				}
			}

			ImGui::Dummy(ImVec2(0, 8));
			{
				ImGui::Checkbox("Raycast Scene", &bRaycastScene);
				if (!bRaycastScene) {
					ImGui::BeginDisabled();
				}
				ImGui::Checkbox("Draw Debug Ray", &Config.Debug.bDrawRayHits);
				if (!bRaycastScene) {
					ImGui::EndDisabled();
				}
			}

			ImGui::Dummy(ImVec2(0, 12));
			ImGui::SeparatorText("Selection");
			{
				std::string Selected = "None";
				if (std::shared_ptr<CActor> Actor = SelectedActor.lock(); Actor != nullptr) {
					Selected = Actor->GetName();
				}
				ImGui::Text("Selected: %s", Selected.c_str());
			}

			ImGui::Dummy(ImVec2(0, 12));
			ImGui::SeparatorText("Serialization");
			{
				UI::FScopedStyle ButtonRounding(ImGuiStyleVar_FrameRounding, 8.0f);
				if (ImGui::Button("Serialize")) {
					Serialize(GameSpec.LevelFilepath);
				}
				ImGui::SameLine();
				if (ImGui::Button("Deserialize")) {
					Deserialize(GameSpec.LevelFilepath);
				}
			}

			ImGui::Spacing();
			UI::Widget::EditorViewportInfo(bEditorViewportFocused, bEditorViewportHovered);

			ImGui::TreePop();
		}

		const bool SceneActive = HasScene();
		if (!SceneActive) {
			ImGui::BeginDisabled();
		}
		if (ImGui::TreeNodeEx("Debug Info", ImGuiTreeNodeFlags_SpanAvailWidth)) {
			ImGui::Text("Last Scene Filepath: %s", std::filesystem::relative(LastSceneFilepath, PROJECT_DIR).generic_string().c_str());
			UI::SetTooltip(LastSceneFilepath.generic_string());
			ImGui::Text("Scene To Open: %s", std::filesystem::relative(SceneToOpen, PROJECT_DIR).generic_string().c_str());
			UI::SetTooltip(SceneToOpen.generic_string());
			ImGui::Text("Open Scene Next Tick: %s", bOpenSceneNextTick ? "Yes" : "No");

			UI::BeginPropertyGrid();

			ImGui::TableNextRow();
			UI::Widget::DragFloat2("Gravity", GRAVITY, 0.0f, 0.010f);

			ImGui::TableNextRow();
			UI::Widget::DragFloat2("Player Spawn", PLAYER_SPAWN, 0.0f, 0.010f);

			ImGui::TableNextRow();
			UI::Widget::DragFloat("Initial Camera Zoom", SCENE_LOAD_CAMERA_ZOOM, 0.01f, 0.0f, 1.0f);

			ImGui::TableNextRow();
			ImGui::Spacing();
			UI::Checkbox("Draw Circle", bDrawCircle);

			ImGui::TableNextRow();
			ImGui::Spacing();
			UI::Checkbox("Draw Circle Filled", bDrawCircleFilled);

			ImGui::TableNextRow();
			ImGui::Spacing();
			UI::Checkbox("Draw Line", bDrawLine);

			ImGui::TableNextRow();
			ImGui::Spacing();
			UI::Widget::DragFloat3("P0", P1, 0.0f, 0.010f);

			ImGui::TableNextRow();
			UI::Widget::DragFloat("Radius", DebugRadius, 0.010f, 0.0f, 10.0f);

			UI::EndPropertyGrid();
			ImGui::TreePop();
		}
		if (!SceneActive) {
			ImGui::EndDisabled();
		}

		UI::Font::Pop();
		UI::End(); /* Sidebar2 */

		/* BottomBar */
		UI::PrepareBottomBar();
		if (UI::Begin(UI::PanelID::BottomBar)) {
			UI::End();
		}
	}

	void CEditor::UI_Player()
	{
		UI::PlayerData(Player);
	}

	void CEditor::UI_ViewportTexture()
	{
		const ImVec2 WindowSize = {
			static_cast<float>(EditorViewportWidth),
			static_cast<float>(EditorViewportHeight)};

		std::shared_ptr<CFramebuffer> Framebuffer = CRenderer::GetViewportFramebuffer();
		std::shared_ptr<CTexture> ViewportTexture = Framebuffer->GetImage(0);

		ImGui::Image(
			static_cast<ImTextureID>(ViewportTexture->GetID()),
			WindowSize,
			ImVec2(0, 1),       /* UV0 */
			ImVec2(1, 0),       /* UV1 */
			ImVec4(1, 1, 1, 1), /* Tint Color   */
			ImVec4(1, 1, 1, 0)  /* Border Color */
		);
	}

	void CEditor::UI_DrawGizmo()
	{
		std::shared_ptr<CActor> SelectedRef = SelectedActor.lock();
		if (!SelectedRef) {
			return;
		}

		if (ImGuiWindow* Window = ImGui::FindWindowByName(UI::PanelID::Viewport)) {
			ImGui::Begin(Window->Name, nullptr, UI::CoreViewportFlags | ImGuiWindowFlags_NoScrollbar);

			CCamera& Camera = Player->GetCamera();
			if (UI::DrawGizmo(Gizmo, *SelectedRef, Camera.GetViewMatrix(), Camera.GetProjectionMatrix())) {
				Player->SetAwake(true);
			}

			ImGui::End();
		}
	}

	void CEditor::UI_Topbar()
	{
		static constexpr float WindowHeight = 32.0f; /* ImGui pixel limitation. */
		static constexpr float ButtonSize = 42.0f + 5.0f;
		static constexpr float EdgeOffset = 4.0f;

		auto ToolbarButton = [](const std::shared_ptr<CTexture>& Icon, const ImColor& Tint, float PaddingY = 0.0f)
		{
			const float Height = std::min(static_cast<float>(Icon->GetHeight()), ButtonSize) - PaddingY * 2.0f;
			const float Width = (static_cast<float>(Icon->GetWidth()) / static_cast<float>(Icon->GetHeight()) * Height);
			UI::ShiftCursorY(EdgeOffset);
			const bool Clicked = ImGui::InvisibleButton(UI::GenerateID(), ImVec2(Width, Height));
			UI::DrawButtonImage(Icon, Tint, Tint, Tint, UI::RectOffset(UI::GetItemRect(), 0.0f, PaddingY));
			return Clicked;
		};

		/* Play Icon */
		{
			std::shared_ptr<CTexture> Image;
			uint32_t Color = 0;

			ESceneState SceneState = ESceneState::None;
			if (Scene) {
				SceneState = Scene->GetState();
				if (SceneState == ESceneState::Play) {
					Image = EditorResources.PlayIcon;
					Color = RGBA32::SmoothGreen;
				} else if (SceneState == ESceneState::Pause) {
					Image = EditorResources.PauseIcon;
					Color = RGBA32::Text::Normal;
				}
			} else {
				Image = EditorResources.PlayIcon;
				Color = RGBA32::Text::Disabled;
			}

			ImGui::SetCursorPosX((ImGui::GetWindowSize().x * 0.50f) - ButtonSize * 0.50f);
			if (SceneState == ESceneState::None) {
				ImGui::BeginDisabled();
			}
			if (ToolbarButton(Image, Color)) {
				LK_ASSERT(Scene); /* Button should only be active if a scene is active. */
				if (SceneState == ESceneState::Play) {
					Scene->SetState(ESceneState::Pause);
					bSceneStateChanged = true;
				} else if (SceneState == ESceneState::Pause) {
					Scene->SetState(ESceneState::Play);
					bSceneStateChanged = true;
				}
			}
			if (SceneState == ESceneState::None) {
				ImGui::EndDisabled();
			}
		}
	}

	void CEditor::UI_MainMenubar()
	{
		ImGui::BeginMenuBar();
		if (ImGui::MenuItem("Settings")) {
			if (UI::IsPauseMenuOpen()) {
				UI::ClosePauseMenu(UI::EPauseMenuView::Default);
			} else {
				UI::OpenPauseMenu(UI::EPauseMenuView::Settings);
			}
		}

		if (ImGui::BeginMenu("Editor")) {
			if (ImGui::MenuItem("Save", "Ctrl+S")) {
				Serialize(GameSpec.LevelFilepath);
			}

			if (ImGui::MenuItem("Close")) {
				Core::Global.RemoveLayer(Core::ELayer::Editor);
			}
			ImGui::EndMenu();
		}

		const bool HasValidScene = (Scene != nullptr);
		if (ImGui::BeginMenu("Scene")) {
			if (HasValidScene) {
				ImGui::BeginDisabled();
			}
			if (ImGui::MenuItem("Open")) {
				if (!Scene) {
					bOpenSceneNextTick = true;
				}
			}
			if (HasValidScene) {
				ImGui::EndDisabled();
			}

			if (!HasValidScene) {
				ImGui::BeginDisabled();
			}
			if (ImGui::MenuItem("Close")) {
				if (Scene) {
					bCloseSceneNextTick = true;
				}
			}
			if (ImGui::MenuItem("Save")) {
				SaveScene();
			}
			if (!HasValidScene) {
				ImGui::EndDisabled();
			}

			ImGui::EndMenu(); /* ~Scene */
		}

		ImGui::EndMenuBar();
	}

	void CEditor::UI_LeftSidebar()
	{
		UI::PrepareLeftSidebar();
		if (!UI::Begin(UI::PanelID::Sidebar1)) {
			return;
		}

		if (Scene) {
			if (Player) {
				UI_Player();
			}
		}
		UI::End();
	}

	void CEditor::UI_LevelLauncher()
	{
		UI::FScopedStyleStack StyleStack(
			ImGuiStyleVar_ItemInnerSpacing, ImVec2(12, 12),
			ImGuiStyleVar_FrameRounding, 12.0f);
		UI::FScopedFont ButtonFont(EFont::Roboto, EFontSize::Header, EFontModifier::Bold);

		static constexpr ImVec2 ButtonSize(292.0f, 74.0f);
		const ImVec2 Avail = ImGui::GetContentRegionAvail();
		UI::ShiftCursorY(Avail.y * 0.25f);
		UI::BannerTextCentralized("Levels", EFont::SourceSansPro, EFontModifier::Bold);
		UI::ShiftCursorY(Avail.y * 0.15f);

		/* Button: Test Level */
		{
			UI::ShiftCursorX((Avail.x * 0.50f) - (ButtonSize.x * 0.50f));
			UI::FScopedColor ButtonColor(ImGuiCol_Button, RGBA32::DarkCyan);
			if (ImGui::Button("Test Level", ButtonSize)) {
				SceneToOpen = std::filesystem::path(SCENES_DIR "/TestLevel.lscene");
				bOpenSceneNextTick = true;
			}
		}

		/* Button: Lukkelele's World */
		{
			UI::ShiftCursorX((Avail.x * 0.50f) - (ButtonSize.x * 0.50f));
			UI::FScopedColor ButtonColor(ImGuiCol_Button, RGBA32::DarkCyan);
			if (ImGui::Button("Lukkelele's World", ButtonSize)) {
				SceneToOpen = std::filesystem::path(SCENES_DIR "/LukkelelesWorld.lscene");
				bOpenSceneNextTick = true;
			}
		}
	}

	void CEditor::OnWindowResized(const uint16_t InWidth, const uint16_t InHeight)
	{
		LK_TRACE_TAG("Editor", "Window resized: ({}, {})", ViewportWidth, ViewportHeight);
		ViewportWidth = InWidth;
		ViewportHeight = InHeight;
	}

	void CEditor::OnKeyPressed(const FKeyData& Data)
	{
		switch (Data.Key) {
			case EKey::Q:
				Gizmo = -1;
				break;
			case EKey::W:
				Gizmo = ImGuizmo::OPERATION::TRANSLATE;
				break;
			case EKey::E:
				Gizmo = ImGuizmo::OPERATION::ROTATE;
				break;
#if 0 /* SCALING NEEDS TO BE SUPPORTED */
			case EKey::R:
				Gizmo = ImGuizmo::OPERATION::SCALE;
				break;
#endif
			case EKey::S:
				if (Data.State == EKeyState::Pressed) {
					if (CKeyboard::IsKeyDown(EKey::LeftControl)) {
						Serialize(GameSpec.LevelFilepath);
					}
				}
				break;
			case EKey::P:
				if (Data.State == EKeyState::Pressed) {
					if (IsGamePaused()) {
						ResumeGame();
					} else {
						PauseGame();
					}
				}
				break;
			case EKey::Escape:
				if (Data.State == EKeyState::Pressed) {
					UI::TogglePauseMenu();
				}
				break;
			case EKey::GraveAccent:
				if (Data.State == EKeyState::Pressed) {
					SelectedActor.reset();
					CSelectionContext::Select(LUUID::Null);
					UI::ChainCreator.OnDeselect();
				}
				break;
		}
	}

	void CEditor::OnMouseButtonPressed(const FMouseButtonData& Data)
	{
		switch (Data.State) {
			case EMouseButtonState::Pressed:
				if (Data.Button == EMouseButton::Button0) {
					if (bEditorViewportFocused) {
						MousePickScene();
					}
				}
				break;
			case EMouseButtonState::Released:
				break;
			case EMouseButtonState::Held:
				break;
		}
	}

	void CEditor::MousePickScene()
	{
		CCamera* Camera = GetActiveCamera();
		if (!Scene || !Camera) {
			return;
		}

		static std::vector<FHitResult> HitResults;
		const uint16_t Picked = PickSceneAtMouse(Scene, HitResults);
		if (Picked > 0) {
			const FHitResult& Hit = HitResults.at(0);
			if (std::shared_ptr<CActor> Ref = Hit.Ref.lock(); Ref != nullptr) {
				CSelectionContext::Select(Ref->GetHandle());
				SelectedActor = Ref;
			}
		}
	}

	void CEditor::RaycastScene()
	{
		CCamera* Camera = GetActiveCamera();
		if (!Scene || !Camera) {
			return;
		}

		static std::vector<FHitResult> HitResults;
		const uint16_t Hits = RaycastScene(Scene, HitResults);
#if 0 /* Disable for now since selection is overkill for raycasts */
		if (Hits > 0) {
			const FHitResult& Hit = HitResults.at(0);
			if (std::shared_ptr<CActor> Ref = Hit.Ref.lock(); Ref != nullptr) {
				SelectedActor = Ref;
			}
		}
#endif
	}

	void CEditor::OpenScene(const std::filesystem::path& ScenePath)
	{
		if (ScenePath.empty()) {
			LK_ERROR_TAG("Editor", "No scene to open");
			return;
		}
		if (Scene) {
			LK_ERROR_TAG("Editor", "A scene is already open");
			return;
		}

		CPhysicsWorld::Initialize(GRAVITY);

		Scene = std::make_shared<CScene>("Editor");
		Scene->Deserialize(ScenePath);
		Scene->SetState(ESceneState::Play);
		CreatePlayer();
		LK_VERIFY(Player);
		CPhysicsWorld::SetPreSolve(PreSolve, Player.get());

		CCamera* Camera = GetActiveCamera();
		LK_VERIFY(Camera);
		Camera->SetZoom(SCENE_LOAD_CAMERA_ZOOM);

		std::shared_ptr<CFramebuffer> Framebuffer = CRenderer::GetViewportFramebuffer();
		Framebuffer->GetImage(0)->Invalidate();
		Framebuffer->Invalidate();

		CWindow::Get()->SetTitle(Format("platformer2d - Editor - {} ({})", Scene->GetName(), Core::GetPlatformName()));
		SceneToOpen.clear();
	}

	void CEditor::CloseScene()
	{
		if (!Scene) {
			LK_WARN_TAG("Editor", "Cannot close scene, none is active");
			return;
		}

		UI::ClosePauseMenu();
		SaveScene();

		LK_TRACE_TAG("Editor", "Release current scene and player");
		Scene.reset();
		Scene = nullptr;
		Player.reset();
		Player = nullptr;

		GRAVITY_CACHED = CPhysicsWorld::GetGravity();
		CPhysicsWorld::Destroy();

		std::shared_ptr<CFramebuffer> Framebuffer = CRenderer::GetViewportFramebuffer();
		Framebuffer->GetImage(0)->Invalidate();
		Framebuffer->Invalidate();

		CWindow::Get()->SetTitle(Format("platformer2d ({})", Core::GetPlatformName()));
		LK_DEBUG_TAG("Editor", "Scene closed");
	}

	void CEditor::SaveScene()
	{
		if (!Scene) {
			LK_WARN_TAG("Editor", "Cannot save scene, none is active");
			return;
		}

		std::filesystem::path ScenePath = Scene->GetFilepath();
		LK_INFO_TAG("Editor", "Save scene: {}", ScenePath);
		LastSceneFilepath = ScenePath;
		if (SceneToOpen.empty()) {
			SceneToOpen = LastSceneFilepath;
		}
		Scene->Serialize(ScenePath);
	}

	void CEditor::OnPickupEvent(CPlayer& InPlayer, const FInteractionComponent& IC)
	{
		const auto& Data = std::get<FPickupInteraction>(IC.GetData());
		switch (Data.Kind) {
			case EPickupKind::Item:
				OnPickupEvent_Item(Data, InPlayer);
				break;
			case EPickupKind::Weapon:
				OnPickupEvent_Rifle(Data, InPlayer);
				break;
		}
	}

	void CEditor::OnPickupEvent_Item(const FPickupInteraction& Interaction, CPlayer& InPlayer)
	{
		const auto& Object = std::get<FPickupItem>(Interaction.Object);
		LK_WARN("Item={} ExpireOnPickup={}", Enum::ToString(Object.Type), Interaction.bExpireWhenPickedUp);
	}

	void CEditor::OnPickupEvent_Rifle(const FPickupInteraction& Interaction, CPlayer& InPlayer)
	{
		const auto& Object = std::get<FPickupWeapon>(Interaction.Object);
		const auto& Spec = std::get<FRifleSpecification>(Object.Spec);
		LK_TRACE("Pickup Weapon={} MagazineSize={} ExpireOnPickup={}", Enum::ToString(Object.Type), Spec.MagazineSize, Interaction.bExpireWhenPickedUp);
		CInventory& Inventory = InPlayer.GetInventory();
		if (Inventory.IsEmpty()) {
			std::shared_ptr<CRifle> Rifle = std::make_shared<CRifle>(Spec, &InPlayer);
			Inventory.AddItem(Rifle);
		} else {
			LK_WARN_TAG("Editor", "Inventory not empty");
		}
	}

	bool PreSolve(b2ShapeId ShapeA, b2ShapeId ShapeB, b2Vec2 Point, b2Vec2 Normal, void* Ctx)
	{
		LK_ASSERT(b2Shape_IsValid(ShapeA) && b2Shape_IsValid(ShapeB));
		if (!Ctx) {
			return false;
		}

		CPlayer& Player = *static_cast<CPlayer*>(Ctx);
		const b2ShapeId PlayerShapeID = Player.GetBody()->GetShapeID();

		const bool InvolvesPlayer = B2_ID_EQUALS(ShapeA, PlayerShapeID) || B2_ID_EQUALS(ShapeB, PlayerShapeID);
		if (!InvolvesPlayer) {
			return true; /* Enable normal contacts. */
		}

		const CActor* ActorA = static_cast<CActor*>(b2Shape_GetUserData(ShapeA));
		const CActor* ActorB = static_cast<CActor*>(b2Shape_GetUserData(ShapeB));

		/* Make normal point from platform to player. */
		if (B2_ID_EQUALS(ShapeA, PlayerShapeID)) {
			Normal.x = -Normal.x;
			Normal.y = -Normal.y;
		}

		const b2Vec2 Up = {0.0f, 1.0f};
		const float UpDot = Normal.x * Up.x + Normal.y * Up.y;
		if (UpDot <= 0.0f) {
			/* Side/ceiling/backface -> behave as a solid. */
			return true;
		}

		const b2BodyId PlayerBody = Player.GetBody()->GetID();
		const b2Vec2 V = b2Body_GetLinearVelocity(PlayerBody);
		const float Vn = V.x * Normal.x + V.y * Normal.y;
		if (Vn > 0.0f) {
			/* Moving along the normal (from below toward the platform) -> ignore contact. */
			return false;
		}

		return true;
	}

}
