#include "editor.h"

#include <system_error>

#include <nfd.hpp>

#include "core/profiler.h"
#include "core/settings.h"
#include "core/window.h"
#include "core/selectioncontext.h"
#include "core/string.h"
#include "core/input/keyboard.h"
#include "core/input/mouse.h"
#include "core/math/math.h"
#include "game/enemy.h"
#include "game/gameplaysystem.h"
#include "game/healthsystem.h"
#include "game/player.h"
#include "game/spawner.h"
#include "game/controller/patrolcontroller.h"
#include "renderer/renderer.h"
#include "renderer/debugrenderer.h"
#include "renderer/ui/editor_resources.h"
#include "renderer/ui/enemytools.h"
#include "renderer/ui/pausemenu.h"
#include "renderer/ui/quickcreator.h"
#include "renderer/ui/selectionpanel.h"
#include "renderer/ui/spriteinspector.h"
#include "renderer/ui/terraincreator.h"
#include "renderer/ui/textureinspector.h"
#include "renderer/ui/ui.h"
#include "renderer/ui/widgets.h"
#include "physics/body.h"
#include "physics/physicsworld.h"
#include "physics/ray.h"
#include "serialization/serialization.h"

namespace platformer2d {

	static constexpr float UI_BG_ALPHA = 0.70f;
	static constexpr const char* UI_ID_LEVEL = "Level";
	static constexpr const char* UI_ID_PLAYER = "Player";
	static int GizmoOp = ImGuizmo::OPERATION::TRANSLATE;
	static bool SceneBrowserCacheDirty = true;

	namespace {
		const FGameSpecification GameSpec = {
			/* clang-format off */
			.InstanceName = "Editor",
			.LevelFilepath = std::filesystem::path(LEVELS_DIR "/editor.yaml"),
			.Player = {
				.ActorSpec = FActorSpecification(ETexture::Player, "Player"),
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
	}

	static std::weak_ptr<CActor> SelectedActor;
	static std::weak_ptr<CActor> RotatingPlatform;

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

	void CEditor::OnInitialize()
	{
		LK_DEBUG_TAG("Editor", "OnInitialize");
		DelegateHandles.OnMouseScroll = CMouse::OnScrollEvent.Add(this, &CEditor::OnMouseScroll);
		EditorResources.Initialize();
	}

	void CEditor::OnShutdown()
	{
		LK_DEBUG_TAG("Editor", "OnShutdown");
		CMouse::OnScrollEvent.Remove(DelegateHandles.OnMouseScroll);
		EditorResources.Destroy();
	}

	void CEditor::OnSceneOpened()
	{
		LK_VERIFY(Scene);
		LK_DEBUG_TAG("Editor", "OnSceneOpened");
		const float InitWidth = (EditorViewportWidth > 0) ? static_cast<float>(EditorViewportWidth) : SCREEN_WIDTH;
		const float InitHeight = (EditorViewportHeight > 0) ? static_cast<float>(EditorViewportHeight) : SCREEN_HEIGHT;

		FActorSpecification CameraSpec;
		CameraSpec.Name = "EditorCamera";
		EditorCamera = std::make_shared<CActor>(CameraSpec);
		auto& CamComp = EditorCamera->AddComponent<FCameraComponent>();
		auto Camera = std::make_shared<CEditorCamera>(InitWidth, InitHeight);
		CamComp.Camera = Camera;

		const glm::vec2 InitPos = (bHasSavedEditorCameraState ? EditorCameraSavedPos : LevelData.PlayerSpawn);
		const float InitZoom = (bHasSavedEditorCameraState ? EditorCameraSavedZoom : LevelData.SceneLoadCameraZoom);
		Camera->SetPosition(InitPos);
		Camera->SetZoom(InitZoom);
		Camera->SetLerpEnabled(PendingEditorCameraLerp);
		Camera->SetActive(true);
		bUseEditorCamera = true;

		if (bSerializeOnNextSceneOpened) {
			bSerializeOnNextSceneOpened = false;
			Serialize(GetSpecification().LevelFilepath);
		}

		/* Find any spawnpoints. */
		std::vector<std::shared_ptr<CActor>> Spawnpoints;
		if (Scene->GetAllWithFlags(EActorFlag_Spawnpoint, Spawnpoints) > 0) {
			GetSystem<CGameplaySystem>().Teleport(Player, Spawnpoints.at(0)->GetPosition());
		}

		if (CDebugRenderer::DebugDraw) {
			if (auto* ActiveCamera = GetActiveCamera()) {
				CDebugRenderer::SetDrawBounds(ActiveCamera->GetPosition(), ActiveCamera->GetHalfSize());
			}

			/* Pass scene to Box2D callbacks. */
			CDebugRenderer::DebugDraw->context = Scene.get();
		}
	}

	static bool IsSameScenePath(const std::filesystem::path& Lhs, const std::filesystem::path& Rhs)
	{
		std::error_code Ec;
		const std::filesystem::path L = std::filesystem::weakly_canonical(Lhs, Ec);
		if (Ec) {
			return false;
		}
		const std::filesystem::path R = std::filesystem::weakly_canonical(Rhs, Ec);
		if (Ec) {
			return false;
		}
		return L == R;
	}

	void CEditor::SwitchToScene(const std::filesystem::path& NewPath)
	{
		if (NewPath.empty()) {
			LK_WARN_TAG("Editor", "Cannot switch to scene, path is empty");
			return;
		}
		if (!std::filesystem::exists(NewPath)) {
			LK_ERROR_TAG("Editor", R"(Cannot switch to scene, file does not exist: "{}")", NewPath);
			return;
		}
		if (Scene && IsSameScenePath(NewPath, Scene->GetFilepath())) {
			LK_TRACE_TAG("Editor", R"(Already editing "{}")", NewPath);
			return;
		}

		LK_INFO_TAG("Editor", "Switching scene: {}", StringUtils::GetPathRelativeToProject(NewPath));
		if (Scene) {
			bCloseSceneNextTick = true;
		}
		SceneToOpen = NewPath;
		bOpenSceneNextTick = true;
		bSerializeOnNextSceneOpened = true;
	}

	void CEditor::OnSceneClosing()
	{
		if (CEditorCamera* EC = GetEditorCamera()) {
			EditorCameraSavedPos = EC->GetPosition();
			EditorCameraSavedZoom = EC->GetZoom();
			bHasSavedEditorCameraState = true;
			EC->SetActive(false);
		}

		SaveScene();
		EditorCamera.reset();
		bUseEditorCamera = true;

		if (CDebugRenderer::DebugDraw) {
			CDebugRenderer::DebugDraw->context = nullptr;
		}
	}

	void CEditor::OnPreTick(const float InDeltaTime)
	{
		LK_PROFILER_SCOPED();
		if (bPendingViewportResize) {
			CRenderer::GetViewportFramebuffer()->Resize(EditorViewportWidth, EditorViewportHeight);
			bPendingViewportResize = false;
		}

		if (!Scene) {
			return;
		}

		if (CEditorCamera* EC = GetEditorCamera(); EC && bUseEditorCamera) {
			EC->SetViewportBounds(EditorViewportBounds[0], EditorViewportBounds[1]);
			EC->Tick(InDeltaTime, bEditorViewportHovered);
		}

		if (std::shared_ptr<CActor> Selected = Scene->GetActor(CSelectionContext::GetSelected())) {
			SelectedActor = Selected;
		}
	}

	void CEditor::OnPostTick(const float InDeltaTime)
	{
		LK_PROFILER_SCOPED();
		if (Scene) {
			CRenderer::DrawText(EFont::SourceSansPro, Scene->GetName(), {-1.3f, 1.1f}, 0.40f, FColor::White, FColor::Black, 1.0f);
		}
		UI::RenderChainPreview(Scene);
		UI::RenderActorPreview(Scene);
		UI::RenderSelectedColliderPreview(Scene);
		UI::RenderEnemySpawnPoints(Scene);
	}

	CCamera* CEditor::GetActiveCamera() const
	{
		if (bUseEditorCamera) {
			return GetEditorCamera();
		}
		return (Player ? &Player->GetCamera() : nullptr);
	}

	CEditorCamera* CEditor::GetEditorCamera() const
	{
		if (!EditorCamera) {
			return nullptr;
		}
		auto& CamComp = EditorCamera->GetComponent<FCameraComponent>();
		LK_ASSERT(CamComp.Camera);
		return static_cast<CEditorCamera*>(CamComp.Camera.get());
	}

	void CEditor::PossessEditorCamera()
	{
		CEditorCamera* EC = GetEditorCamera();
		if (!EC || !Player) {
			return;
		}

		CCamera& PlayerCam = Player->GetCamera();
		if (EC->IsLerpEnabled()) {
			EC->BeginSwitchLerp(PlayerCam.GetPosition(), PlayerCam.GetZoom());
		} else {
			EC->CancelSwitchLerp();
		}

		PlayerCam.SetActive(false);
		EC->SetActive(true);
		bUseEditorCamera = true;
	}

	void CEditor::PossessPlayerCamera()
	{
		CEditorCamera* EC = GetEditorCamera();
		if (!EC || !Player) {
			return;
		}

		CCamera& PlayerCam = Player->GetCamera();
		if (EC->IsLerpEnabled()) {
			PlayerCam.BeginSwitchLerp(EC->GetPosition(), EC->GetZoom());
		} else {
			PlayerCam.CancelSwitchLerp();
		}

		EC->SetActive(false);
		PlayerCam.SetActive(true);
		bUseEditorCamera = false;
	}

	static bool PickAABB(const glm::vec2& MouseWorld, const CActor& Actor, float& OutDistance)
	{
		const glm::vec2 Pos = Actor.GetPosition();
		if (Math::IsPointInPolygon(MouseWorld, Pos, Actor.GetSize(), Actor.GetRotation())) {
			OutDistance = glm::length(MouseWorld - Pos);
			return true;
		}
		return false;
	}

	static bool PickChain(const FChain& Chain, const glm::vec2& Origin, const glm::vec2& Point, const float Threshold)
	{
		const std::size_t N = Chain.Points.size();
		if (N < 2) {
			return false;
		}

		const std::size_t Last = Chain.bLoop ? N : (N - 1);
		for (std::size_t Idx = 0; Idx < Last; Idx++) {
			const glm::vec2 A = Origin + Chain.Points[Idx];
			const glm::vec2 B = Origin + Chain.Points[(Idx + 1) % N];
			const glm::vec2 AB = B - A;
			const float Len2 = glm::dot(AB, AB);
			const float T = (Len2 > 0.0f) ? glm::clamp(glm::dot(Point - A, AB) / Len2, 0.0f, 1.0f) : 0.0f;
			if (glm::length(Point - (A + T * AB)) <= Threshold) {
				return true;
			}
		}

		return false;
	}

	std::uint16_t CEditor::PickSceneAtMouse(std::shared_ptr<CScene> TargetScene, std::vector<FHitResult>& HitResults)
	{
		LK_PROFILER_SCOPED();
		HitResults.clear();
		const CCamera& Camera = *GetActiveCamera();
		const glm::vec2 MouseWorld = GetMouseInWorldSpace(Camera);
		if (!std::isfinite(MouseWorld.x) || !std::isfinite(MouseWorld.y)) {
			return 0;
		}

		constexpr float ChainPickThreshold = 0.03f;
		for (const auto& Actor : TargetScene->GetActors()) {
			float Distance = 0.0f;
			bool Hit = false;

			if (const CBody* Body = Actor->GetBody(); Body != nullptr) {
				std::visit([&]<typename T>(const T& Shape)
				{
					if constexpr (std::is_same_v<T, FChain>) {
						const glm::vec2 Origin = Body->GetPosition();
						if (PickChain(Shape, Origin, MouseWorld, ChainPickThreshold)) {
							Hit = true;
							Distance = glm::length(MouseWorld - Origin);
						}
					} else if constexpr (std::is_same_v<T, FPolygon>
						|| std::is_same_v<T, FCapsule>
						|| std::is_same_v<T, FLine>) {
						Hit = PickAABB(MouseWorld, *Actor, Distance);
					}
				}, Body->GetShape());
			} else {
				Hit = PickAABB(MouseWorld, *Actor, Distance);
			}

			if (Hit) {
				FHitResult Entry{};
				Entry.Handle = Actor->GetHandle();
				Entry.Ref = Actor;
				Entry.Distance = Distance;
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
		return static_cast<std::uint16_t>(HitResults.size());
	}

	bool CEditor::Serialize(const std::filesystem::path& OutFile) const
	{
		LK_DEBUG_TAG("Editor", "Serialize: {}", OutFile);
		YAML::Emitter Out;

		Out << YAML::BeginMap;
		Out << YAML::Key << "Level" << YAML::Value << LayerName;

		LK_TRACE("LastSceneFilepath: {}", LastSceneFilepath);
		const std::filesystem::path ScenePath = (Scene ? Scene->GetFilepath() : LastSceneFilepath);
		LK_DEBUG_TAG("Editor", "Scene path: {}", ScenePath);
		Out << YAML::Key << "Scene" << YAML::Value << Core::GetRelativeFromProject(ScenePath);

		Out << YAML::Key << "PlayerSpawn" << YAML::Value << LevelData.PlayerSpawn;
		Out << YAML::Key << "CameraZoom" << YAML::Value << LevelData.SceneLoadCameraZoom;

		const CEditorCamera* EC = GetEditorCamera();
		const glm::vec2 EditorCamPos = EC ? EC->GetPosition() : EditorCameraSavedPos;
		const float EditorCamZoom = EC ? EC->GetZoom() : EditorCameraSavedZoom;
		const bool EditorCamLerp = EC ? EC->IsLerpEnabled() : PendingEditorCameraLerp;
		Out << YAML::Key << "EditorCameraPos" << YAML::Value << EditorCamPos;
		Out << YAML::Key << "EditorCameraZoom" << YAML::Value << EditorCamZoom;
		Out << YAML::Key << "EditorCameraLerp" << YAML::Value << EditorCamLerp;

		Out << YAML::Key << "Physics";
		Out << YAML::BeginMap;
		Out << YAML::Key << "Gravity" << YAML::Value << (Scene ? CPhysicsWorld::GetGravity() : LevelData.CachedGravity);
		Out << YAML::EndMap;

		Out << YAML::EndMap;

		if (Scene) {
			Scene->Serialize(ScenePath);
		}

		std::ofstream File(OutFile);
		File << Out.c_str();

		return true;
	}

	bool CEditor::Deserialize(const std::filesystem::path& Filepath)
	{
		LK_DEBUG_TAG("Editor", "Deserialize: {}", StringUtils::GetPathRelativeToProject(Filepath));
		LK_ASSERT(std::filesystem::exists(Filepath), "Filepath does not exist: {}", Filepath);
		if (!std::filesystem::exists(Filepath)) {
			LK_ERROR_TAG("Editor", "Failed to deserialize, path does not exist: {}", Filepath);
			return false;
		}

		std::ifstream InputStream(Filepath);
		std::stringstream StringStream;
		StringStream << InputStream.rdbuf();
		const std::string YamlString = StringStream.str();

		const YAML::Node Data = YAML::Load(YamlString);

		Serialization::DeserializeProperty("PlayerSpawn", LevelData.PlayerSpawn, glm::vec2(0.0f, 0.0f), Data);
		Serialization::DeserializeProperty("CameraZoom", LevelData.SceneLoadCameraZoom, 0.40f, Data);
		LevelData.CachedGravity = LevelData.Gravity;

		if (Data["EditorCameraPos"]) {
			Serialization::DeserializeProperty("EditorCameraPos", EditorCameraSavedPos, LevelData.PlayerSpawn, Data);
			Serialization::DeserializeProperty("EditorCameraZoom", EditorCameraSavedZoom, LevelData.SceneLoadCameraZoom, Data);
			bHasSavedEditorCameraState = true;
		}
		bool LerpEnabledInit = true;
		Serialization::DeserializeProperty("EditorCameraLerp", LerpEnabledInit, true, Data);
		if (CEditorCamera* EC = GetEditorCamera()) {
			EC->SetLerpEnabled(LerpEnabledInit);
		}
		PendingEditorCameraLerp = LerpEnabledInit;

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
		LK_PROFILER_SCOPED();
		UpdateEditorViewportBounds();
		bEditorViewportFocused = ImGui::IsWindowFocused();
		bEditorViewportHovered = ImGui::IsWindowHovered();

		const glm::vec2 MousePos = CMouse::GetPos();
		bEditorViewportHovered = (MousePos.x >= EditorViewportBounds[0].x) && (MousePos.y >= EditorViewportBounds[0].y)
			&& (MousePos.x <= EditorViewportBounds[1].x) && (MousePos.y <= EditorViewportBounds[1].y);
	}

	void CEditor::UpdateEditorViewportBounds()
	{
		LK_PROFILER_SCOPED();
		const ImVec2 WindowPos = ImGui::GetCurrentWindow()->Pos;
		const ImVec2 RegionMin = ImGui::GetWindowContentRegionMin();
		const ImVec2 RegionMax = ImGui::GetWindowContentRegionMax();
		EditorViewportBounds[0] = {WindowPos.x + RegionMin.x, WindowPos.y + RegionMin.y};
		EditorViewportBounds[1] = {WindowPos.x + RegionMax.x, WindowPos.y + RegionMax.y};

		const float VpWidth = EditorViewportBounds[1].x - EditorViewportBounds[0].x;
		const float VpHeight = EditorViewportBounds[1].y - EditorViewportBounds[0].y;
		if ((EditorViewportWidth != static_cast<std::uint16_t>(VpWidth)) || (EditorViewportHeight != static_cast<std::uint16_t>(VpHeight))) {
			EditorViewportWidth = static_cast<std::uint16_t>(VpWidth);
			EditorViewportHeight = static_cast<std::uint16_t>(VpHeight);
			bPendingViewportResize = true;
		}
	}

	void CEditor::UpdateViewportBounds()
	{
		ViewportBounds[0] = {0.0f, 0.0f};
		ViewportBounds[1] = CWindow::Get().GetSize();
	}

	glm::vec2 CEditor::GetMouseInViewportSpace()
	{
		glm::vec2 MousePos = CMouse::GetPos();
		MousePos.x -= EditorViewportBounds[0].x;
		MousePos.y -= EditorViewportBounds[0].y;
		const float VpWidth = EditorViewportBounds[1].x - EditorViewportBounds[0].x;
		const float VpHeight = EditorViewportBounds[1].y - EditorViewportBounds[0].y;

		return glm::vec2(
			(MousePos.x / static_cast<float>(VpWidth)) * 2.0f - 1.0f,
			((MousePos.y / static_cast<float>(VpHeight)) * 2.0f - 1.0f) * -1.0f);
	}

	void CEditor::OnActorCreated(const LUUID Handle, std::weak_ptr<CActor> ActorRef)
	{
		if (!Scene) {
			return;
		}
		if (std::shared_ptr<CActor> Actor = ActorRef.lock(); Actor != nullptr) {
			LK_TRACE_TAG("Editor", "OnActorCreated: {} ({})", Actor->GetName(), Handle);
			LK_ASSERT(Scene);
			UpdateInputBuffer(Scene->GetActors().size());
		}
	}

	void CEditor::OnActorDeleted(const LUUID Handle)
	{
		if (!Scene) {
			return;
		}
		LK_DEBUG_TAG("Editor", "OnActorDeleted: {}", Handle);
		UpdateInputBuffer(Scene->GetActors().size());
		UI::Actor::OnActorDeleted(Handle);
		UI::SetSpawnPointVisible(Handle, false);
	}

	void CEditor::OnKey(const FKeyData& Data)
	{
		switch (Data.Key) {
			case EKey::Q:
				if (Data.State == EKeyState::Pressed) {
					if (bEditorViewportFocused) {
						GizmoOp = -1;
					}
				}
				break;
			case EKey::W:
				if (Data.State == EKeyState::Pressed) {
					if (bEditorViewportFocused) {
						GizmoOp = ImGuizmo::OPERATION::TRANSLATE;
					}
				}
				break;
			case EKey::E:
				if (Data.State == EKeyState::Pressed) {
					if (bEditorViewportFocused) {
						GizmoOp = ImGuizmo::OPERATION::ROTATE;
					}
				}
				break;
			case EKey::R:
				if (Data.State == EKeyState::Pressed) {
					if (bEditorViewportFocused) {
						GizmoOp = ImGuizmo::OPERATION::SCALE;
					}
				}
				break;
			case EKey::S:
				if (Data.State == EKeyState::Pressed) {
					if (bEditorViewportFocused) {
						if (CKeyboard::IsKeyDown(EKey::LeftControl)) {
							Serialize(GetSpecification().LevelFilepath);
						}
					}
				}
				break;
			case EKey::P:
				if (Data.State == EKeyState::Pressed) {
					if (bEditorViewportFocused) {
						if (IsGamePaused()) {
							ResumeGame();
						} else {
							PauseGame();
						}
					}
				}
				break;
			case EKey::Space:
				if (Data.State == EKeyState::Pressed) {
					if (CKeyboard::IsKeyDown(EKey::LeftControl) || CKeyboard::IsKeyDown(EKey::RightControl)) {
						if (IsGamePaused()) {
							ResumeGame();
						} else {
							PauseGame();
						}
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
					if (bEditorViewportFocused) {
						SelectedActor.reset();
						CSelectionContext::Select(LUUID::Null);
						UI::ActorAttr.bPreviewSelected = false;
						UI::TerrainCreator.OnDeselect();
					}
				}
				break;
		}

		if (Player) {
			Player->OnKey(Data);
		}
	}

	void CEditor::HandleViewportLeftClick()
	{
		if (UI::ActorAttr.bPreviewSelected && ImGuizmo::IsOver()) {
			return;
		}

		bool HitPreview = false;
		if (UI::ActorAttr.bPreview) {
			if (CCamera* Camera = GetActiveCamera()) {
				const glm::vec2 MouseWorld = GetMouseInWorldSpace(*Camera);
				if (std::isfinite(MouseWorld.x) && std::isfinite(MouseWorld.y)) {
					const glm::vec2 Half = UI::ActorAttr.Size * 0.50f;
					const glm::vec2 P = UI::ActorAttr.Position;
					HitPreview = (MouseWorld.x >= P.x - Half.x) && (MouseWorld.x <= P.x + Half.x)
						&& (MouseWorld.y >= P.y - Half.y) && (MouseWorld.y <= P.y + Half.y);
				}
			}
		}

		if (HitPreview) {
			UI::ActorAttr.bPreviewSelected = true;
			SelectedActor.reset();
			CSelectionContext::Select(LUUID::Null);
			return;
		}

		UI::ActorAttr.bPreviewSelected = false;
		MousePickScene();
		if (Scene) {
			SelectedActor = Scene->GetActor(CSelectionContext::GetSelected());
		}
	}

	void CEditor::OnMouseButton(const FMouseButtonData& Data)
	{
		LK_TRACE_TAG("Editor", "Button={} NewState={}", Enum::ToString(Data.Button), Enum::ToString(Data.State));

		switch (Data.Button) {
			case EMouseButton::Button0:
				if ((Data.State == EMouseButtonState::Pressed) && bEditorViewportFocused) {
					HandleViewportLeftClick();
				}
				break;
			default:
				break;
		}

		if (Player && bEditorViewportHovered) {
			Player->OnMouseButton(Data);
		}
	}

	void CEditor::OnMouseScroll(const EMouseScrollDirection Direction)
	{
		LK_TRACE_TAG("Editor", "{} UseEditorCamera={}", Enum::ToString(Direction), bUseEditorCamera);
		if (bUseEditorCamera) {
			if (auto* EC = GetEditorCamera(); EC && bEditorViewportHovered) {
				EC->OnMouseScroll(Direction);
			}
		}

		if (Player) {
			Player->OnMouseScroll(Direction);
		}

		if (auto* ActiveCamera = GetActiveCamera()) {
			CDebugRenderer::SetDrawBounds(ActiveCamera->GetPosition(), ActiveCamera->GetHalfSize());
		}
	}

	void CEditor::RenderUI()
	{
		LK_PROFILER_SCOPED();
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
				/* Skip if scene is to open to not flicker the screen. */
				if (!bOpenSceneNextTick) {
					UI::LevelLauncher();
				}
			}

			if (Scene) {
				const auto& StatsGraphics = FSettings::Get().Graphics;
				if (StatsGraphics.bShowFPS || StatsGraphics.bShowFrametime || StatsGraphics.bShowDebugStats) {
					UI::Statistics();
				}
				UI::SelectionPanel();
				UI_DrawGizmo();
			}

			UI::End();
		}

		UI_Level();

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

		UI::RenderSpriteInspector();
		UI::RenderTextureInspector();

		UI::End();
	}

	void CEditor::UI_Level()
	{
		LK_PROFILER_SCOPED();
		if (UI::Begin(UI::PanelID::SceneManager)) {
			UI_SceneBrowser();
			UI::Separator(2);
			UI::End();
		}

		UI::SceneManagerPanel(Scene);
		UI::Creator(Scene);
		UI::RenderTerrainCreator(Scene);

		if (UI::Begin(UI::PanelID::SceneManager)) {
			UI::Separator(2);
			UI::EnemyTools(Scene);

			ImGui::Spacing();
			UI::EnemiesInfo(Scene);

			if (Player) {
				ImGui::Spacing();
				UI::Rifle(Player->GetRifle());
			}

			UI::End();
		}

		UI_BottomBar();
	}

	void CEditor::UI_Player()
	{
		LK_PROFILER_SCOPED();
		UI::PlayerData(Player);
	}

	void CEditor::UI_ViewportTexture()
	{
		LK_PROFILER_SCOPED();
		const ImVec2 WindowSize = {static_cast<float>(EditorViewportWidth), static_cast<float>(EditorViewportHeight)};
		std::shared_ptr<CFramebuffer> Framebuffer = CRenderer::GetViewportFramebuffer();
		std::shared_ptr<CTexture> ViewportTexture = Framebuffer->GetImage(0);
		UI::Image(ViewportTexture, WindowSize);
	}

	void CEditor::UI_DrawGizmo()
	{
		LK_PROFILER_SCOPED();
		std::shared_ptr<CActor> SelectedRef = SelectedActor.lock();
		const bool HasSelection = (SelectedRef != nullptr);
		const bool PreviewActive = UI::ActorAttr.bPreview && UI::ActorAttr.bPreviewSelected;
		if (!HasSelection && !PreviewActive) {
			return;
		}

		if (ImGuiWindow* Window = ImGui::FindWindowByName(UI::PanelID::Viewport)) {
			ImGui::Begin(Window->Name, nullptr, UI::CoreViewportFlags | ImGuiWindowFlags_NoScrollbar);
			if (CCamera* Camera = GetActiveCamera()) {
				if (HasSelection) {
					if (UI::DrawGizmo(GizmoOp, *SelectedRef, Camera->GetViewMatrix(), Camera->GetProjectionMatrix())) {
						Player->SetAwake(true);
					}
				} else {
					UI::DrawTranslateGizmo(UI::ActorAttr.Position, Camera->GetViewMatrix(), Camera->GetProjectionMatrix());
				}
			}

			ImGui::End();
		}
	}

	void CEditor::UI_Topbar()
	{
		LK_PROFILER_SCOPED();
		constexpr float WindowHeight = 32.0f;
		constexpr float EdgeOffset = 4.0f;
		static constexpr float ButtonSize = 42.0f + 5.0f;

		auto ToolbarButton = [](const std::shared_ptr<CTexture>& Icon, const ImColor& Tint, float PaddingY = 0.0f)
		{
			const float Height = std::min(static_cast<float>(Icon->GetHeight()), ButtonSize) - PaddingY * 2.0f;
			const float Width = (static_cast<float>(Icon->GetWidth()) / static_cast<float>(Icon->GetHeight()) * Height);
			UI::ShiftCursorY(EdgeOffset);
			const bool Clicked = ImGui::InvisibleButton(UI::GenerateID(), ImVec2(Width, Height));
			UI::DrawButtonImage(Icon, Tint, Tint, Tint, UI::RectOffset(UI::GetItemRect(), 0.0f, PaddingY));
			return Clicked;
		};

		{
			std::shared_ptr<CTexture> Image;
			std::uint32_t Color = 0;

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
				LK_ASSERT(Scene);
				if (SceneState == ESceneState::Play) {
					PauseGame();
				} else if (SceneState == ESceneState::Pause) {
					ResumeGame();
				}
			}
			if (SceneState == ESceneState::None) {
				ImGui::EndDisabled();
			}
		}

		{
			const bool SceneExists = HasScene();
			const bool PlayerActive = !bUseEditorCamera;
			const bool EditorActive = bUseEditorCamera;

			const ImGuiStyle& Style = ImGui::GetStyle();
			const ImVec2 Avail = ImGui::GetContentRegionAvail();

			UI::FScopedFont ScopedFont(EFontSize::Large);
			UI::FScopedStyleStack StyleStack(
				ImGuiStyleVar_FrameRounding, 8,
				ImGuiStyleVar_FramePadding, ImVec2(8, 6));

			const float CamButtonWidth = ImGui::CalcTextSize("Editor Camera").x + (2.0f * (Style.FramePadding.x));
			const float CamButtonHeight = Avail.y;
			constexpr float CamButtonGap = 16.0f;
			ImGui::SameLine();
			UI::ShiftCursorX(80);

			if (!SceneExists) {
				ImGui::BeginDisabled();
			}
			{
				if (PlayerActive) {
					ImGui::PushStyleColor(ImGuiCol_Button, RGBA32::SmoothGreen);
				}
				if (ImGui::Button("Player", ImVec2(CamButtonWidth, CamButtonHeight))) {
					if (SceneExists && bUseEditorCamera) {
						PossessPlayerCamera();
					}
				}
				if (PlayerActive) {
					ImGui::PopStyleColor();
				}
			}
			ImGui::SameLine(0.0f, CamButtonGap);
			{
				if (EditorActive) {
					ImGui::PushStyleColor(ImGuiCol_Button, RGBA32::SmoothGreen);
				}
				if (ImGui::Button("Editor", ImVec2(CamButtonWidth, CamButtonHeight))) {
					if (SceneExists && !bUseEditorCamera) {
						PossessEditorCamera();
					}
				}
				if (EditorActive) {
					ImGui::PopStyleColor();
				}
			}
			if (!SceneExists) {
				ImGui::EndDisabled();
			}
		}
	}

	static bool PickSceneFile(std::filesystem::path& OutPath)
	{
		NFD::Guard NfdGuard;
		NFD::UniquePathU8 Picked;
		static constexpr nfdu8filteritem_t Filters[] = {
			{"Scene", "lscene"}
        };
		const nfdresult_t Result = NFD::OpenDialog(Picked, Filters, 1, SCENES_DIR);
		if (Result != NFD_OKAY) {
			if (Result == NFD_ERROR) {
				const char* Err = NFD::GetError();
				LK_ERROR_TAG("Editor", "NFD error: {}", (Err ? Err : "unknown"));
			}
			return false;
		}
		OutPath = std::filesystem::path(Picked.get());
		return true;
	}

	void CEditor::UI_MainMenubar()
	{
		auto Option = [](const char* Label, bool& Value)
		{
			if (ImGui::MenuItem(Label, nullptr, Value)) {
				Value = !Value;
			}
		};

		ImGui::BeginMenuBar();
		ImGui::PushItemFlag(ImGuiItemFlags_AutoClosePopups, false);
		if (ImGui::MenuItem("Settings")) {
			if (UI::IsPauseMenuOpen()) {
				UI::ClosePauseMenu(UI::EPauseMenuView::Default);
			} else {
				UI::OpenPauseMenu(UI::EPauseMenuView::Settings);
			}
		}

		if (ImGui::BeginMenu("Editor")) {
			if (ImGui::MenuItem("Save", "Ctrl+S")) {
				Serialize(GetSpecification().LevelFilepath);
			}

			if (ImGui::MenuItem("Close")) {
				Core::Global.RemoveLayer(Core::ELayer::Editor);
			}

			ImGui::EndMenu();
		}

		const bool HasValidScene = (Scene != nullptr);
		bool OpenNewScenePopup = false;
		if (ImGui::BeginMenu("Scene")) {
			if (ImGui::MenuItem("New")) {
				OpenNewScenePopup = true;
			}

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

			if (ImGui::MenuItem("Switch")) {
				std::filesystem::path Picked;
				if (PickSceneFile(Picked)) {
					SwitchToScene(Picked);
				}
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

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Window")) {
			Option("Terrain Creator", UI::TerrainCreator.bOpen);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Layout")) {
			const UI::EDockLayout ActiveLayout = UI::GetDockLayout();
			for (const UI::EDockLayout Layout : Enum::View<UI::EDockLayout>()) {
				if (ImGui::MenuItem(Enum::ToString<const char*>(Layout), nullptr, Layout == ActiveLayout)) {
					UI::SetDockLayout(Layout);
				}
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Reset Layout")) {
				UI::ResetDockLayout();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Tools")) {
			if (ImGui::MenuItem("Sprite Inspector", nullptr, UI::IsSpriteInspectorOpen())) {
				UI::ToggleSpriteInspector();
			}
			if (ImGui::MenuItem("Texture Inspector", nullptr, UI::IsTextureInspectorOpen())) {
				UI::ToggleTextureInspector();
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Debug")) {
			const bool DebugRendererValid = (CDebugRenderer::DebugDraw != nullptr);
			if (!DebugRendererValid) {
				ImGui::BeginDisabled();
			}
			if (ImGui::BeginMenu("Physics")) {
				Option("Body Shapes", CDebugRenderer::DebugDraw->drawShapes);
				Option("Body Names", CDebugRenderer::DebugDraw->drawBodyNames);
				Option("Body Mass", CDebugRenderer::DebugDraw->drawMass);
				Option("Contact Forces", CDebugRenderer::DebugDraw->drawContactForces);
				Option("Contact Normals", CDebugRenderer::DebugDraw->drawContactNormals);
				Option("Friction Forces", CDebugRenderer::DebugDraw->drawFrictionForces);
				Option("Joints", CDebugRenderer::DebugDraw->drawJoints);
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Draw")) {
				Option("Text", CDebugRenderer::StringConf.bDraw);
				Option("Circles", CDebugRenderer::CircleConf.bDraw);
				Option("Solid Circles", CDebugRenderer::CircleSolidConf.bDraw);
				Option("Points", CDebugRenderer::PointConf.bDraw);
				Option("Polygons", CDebugRenderer::PolygonConf.bDraw);
				Option("Solid Polygons", CDebugRenderer::PolygonSolidConf.bDraw);
				Option("Solid Capsule", CDebugRenderer::CapsuleSolidConf.bDraw);
				Option("Transforms", CDebugRenderer::TransformConf.bDraw);
				Option("Segments", CDebugRenderer::SegmentConf.bDraw);
				ImGui::EndMenu();
			}
			if (!DebugRendererValid) {
				ImGui::EndDisabled();
			}

			ImGui::EndMenu();
		}
		ImGui::PopItemFlag();
		ImGui::EndMenuBar();

		if (OpenNewScenePopup) {
			NewSceneNameBuf.fill('\0');
			ImGui::OpenPopup("New Scene");
		}
		UI_NewScenePopup();
	}

	void CEditor::UI_NewScenePopup()
	{
		const ImVec2 Center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(Center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(400.0f, 0.0f), ImGuiCond_Appearing);

		if (!ImGui::BeginPopupModal("New Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings)) {
			return;
		}

		ImGui::TextUnformatted("Scene name");
		if (ImGui::IsWindowAppearing()) {
			ImGui::SetKeyboardFocusHere();
		}
		ImGui::SetNextItemWidth(360.0f);
		const bool EnterPressed = ImGui::InputText("##NewSceneName", NewSceneNameBuf.data(), NewSceneNameBuf.size(), ImGuiInputTextFlags_EnterReturnsTrue);

		const std::string Name(NewSceneNameBuf.data());
		const bool NameEmpty = Name.empty();

		std::filesystem::path NewPath;
		bool FileExists = false;
		if (!NameEmpty) {
			NewPath = std::filesystem::path(SCENES_DIR) / (Name + "." + CScene::FILE_EXTENSION);
			std::error_code Ec;
			FileExists = std::filesystem::exists(NewPath, Ec);
		}

		const float HintLineHeight = ImGui::GetTextLineHeightWithSpacing();
		if (FileExists) {
			UI::FScopedColor TextColor(ImGuiCol_Text, ImVec4(1.0f, 0.40f, 0.40f, 1.0f));
			ImGui::TextUnformatted("Scene already exists");
		} else {
			ImGui::Dummy(ImVec2(0.0f, HintLineHeight));
		}

		const bool CanCreate = !NameEmpty && !FileExists;

		constexpr float ButtonW = 96.0f;
		const float Spacing = ImGui::GetStyle().ItemSpacing.x;
		const float Avail = ImGui::GetContentRegionAvail().x;
		ImGui::Dummy(ImVec2(0.0f, 6.0f));
		UI::ShiftCursorX(Avail - (ButtonW * 2.0f + Spacing));

		if (!CanCreate) {
			ImGui::BeginDisabled();
		}
		const bool CreatePressed = ImGui::Button("Create", ImVec2(ButtonW, 0.0f)) || (EnterPressed && CanCreate);
		if (CreatePressed) {
			CScene EmptyScene(Name);
			if (EmptyScene.Serialize(NewPath)) {
				LK_INFO_TAG("Editor", "Created new scene: {}", StringUtils::GetPathRelativeToProject(NewPath));
				SwitchToScene(NewPath);
				SceneBrowserCacheDirty = true; /* Refresh scene browser. */
				ImGui::CloseCurrentPopup();
			} else {
				LK_ERROR_TAG("Editor", "Failed to create scene file: {}", NewPath);
			}
		}
		if (!CanCreate) {
			ImGui::EndDisabled();
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(ButtonW, 0.0f))) {
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
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

		constexpr ImVec2 ButtonSize(292.0f, 74.0f);
		const ImVec2 Avail = ImGui::GetContentRegionAvail();
		UI::ShiftCursorY(Avail.y * 0.25f);
		UI::BannerTextCentralized("Levels", EFont::SourceSansPro, EFontModifier::Bold);
		UI::ShiftCursorY(Avail.y * 0.15f);

		{
			UI::ShiftCursorX((Avail.x * 0.50f) - (ButtonSize.x * 0.50f));
			UI::FScopedColor ButtonColor(ImGuiCol_Button, RGBA32::DarkCyan);
			if (ImGui::Button("Test Level", ButtonSize)) {
				SceneToOpen = std::filesystem::path(SCENES_DIR "/TestLevel.lscene");
				bOpenSceneNextTick = true;
			}
		}

		{
			UI::ShiftCursorX((Avail.x * 0.50f) - (ButtonSize.x * 0.50f));
			UI::FScopedColor ButtonColor(ImGuiCol_Button, RGBA32::DarkCyan);
			if (ImGui::Button("Lukkelele's World", ButtonSize)) {
				SceneToOpen = std::filesystem::path(SCENES_DIR "/LukkelelesWorld.lscene");
				bOpenSceneNextTick = true;
			}
		}
	}

	void CEditor::UI_SceneBrowser()
	{
		static const std::filesystem::path ScenesRoot(SCENES_DIR);
		static std::vector<std::filesystem::path> CachedScenes;

		if (SceneBrowserCacheDirty) {
			CachedScenes.clear();
			std::error_code Ec;
			const std::filesystem::path Root(SCENES_DIR);
			if (std::filesystem::exists(Root, Ec)) {
				for (const auto& Entry : std::filesystem::recursive_directory_iterator(Root, Ec)) {
					if (Ec) {
						break;
					}
					if (Entry.is_regular_file(Ec) && (Entry.path().extension() == ".lscene")) {
						CachedScenes.push_back(Entry.path());
					}
				}
			}
			std::sort(CachedScenes.begin(), CachedScenes.end());
			SceneBrowserCacheDirty = false;
		}

		if (!ImGui::TreeNodeEx("Scenes", ImGuiTreeNodeFlags_SpanAvailWidth)) {
			return;
		}

		{
			UI::FScopedStyle ButtonRounding(ImGuiStyleVar_FrameRounding, 6.0f);
			if (ImGui::Button("Refresh")) {
				SceneBrowserCacheDirty = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("Browse")) {
				std::filesystem::path Picked;
				if (PickSceneFile(Picked)) {
					SwitchToScene(Picked);
				}
			}
		}

		ImGui::Dummy(ImVec2(0, 4));
		const std::filesystem::path ActivePath = Scene ? Scene->GetFilepath() : std::filesystem::path{};
		for (const std::filesystem::path& ScenePath : CachedScenes) {
			const bool IsActive = !ActivePath.empty() && IsSameScenePath(ScenePath, ActivePath);
			std::error_code Ec;
			std::string Display = std::filesystem::relative(ScenePath, ScenesRoot, Ec).generic_string();
			if (Display.empty() || Ec) {
				Display = ScenePath.filename().generic_string();
			}
			Display = Display.substr(0, Display.find(".lscene"));

			const std::string IdString = ScenePath.generic_string();
			UI::FScopedID ScopedID(IdString.c_str());
			if (IsActive) {
				ImGui::PushStyleColor(ImGuiCol_Text, RGBA32::NiceGreen);
			}
			if (ImGui::Selectable(Display.c_str(), IsActive, ImGuiSelectableFlags_DontClosePopups)) {
				if (!IsActive) {
					SwitchToScene(ScenePath);
				}
			}
			if (IsActive) {
				ImGui::PopStyleColor();
			}
			UI::SetTooltip(StringUtils::GetPathRelativeToProject(ScenePath));
		}

		ImGui::TreePop();
	}

	void CEditor::UI_BottomBar()
	{
		UI::PrepareBottomBar();
		if (!UI::Begin(UI::PanelID::BottomBar)) {
			return;
		}

		const ImVec2 Avail = ImGui::GetContentRegionAvail();
		ImGui::BeginChild("##DebugInfoContent", ImVec2(std::max(Avail.x * 0.33f, 580.0f), Avail.y), ImGuiChildFlags_None);

		const bool SceneActive = HasScene();
		if (!SceneActive) {
			ImGui::BeginDisabled();
		}
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNodeEx("Debug Info", ImGuiTreeNodeFlags_Framed)) {
			ImGui::Text("Last Scene Filepath: %s", std::filesystem::relative(LastSceneFilepath, PROJECT_DIR).generic_string().c_str());
			UI::SetTooltip(LastSceneFilepath.generic_string());
			ImGui::Text("Scene To Open: %s", std::filesystem::relative(SceneToOpen, PROJECT_DIR).generic_string().c_str());
			UI::SetTooltip(SceneToOpen.generic_string());
			ImGui::Text("Open Scene Next Tick: %s", bOpenSceneNextTick ? "Yes" : "No");

			UI::BeginPropertyGrid();

			UI::Table::NextRow();
			UI::DragFloat2("Gravity", LevelData.Gravity, 0.0f, 0.010f);

			UI::Table::NextRow();
			UI::DragFloat2("Player Spawn", LevelData.PlayerSpawn, 0.0f, 0.010f);

			UI::Table::NextRow();
			UI::DragFloat("Initial Camera Zoom", LevelData.SceneLoadCameraZoom, 0.01f, 0.0f, 1.0f);

			UI::Table::NextRow();
			int LineW = static_cast<int>(CDebugRenderer::SegmentConf.LineWidth);
			if (ImGui::DragInt("Segment Line Width", &LineW, 1, 1, 12)) {
				CDebugRenderer::SegmentConf.LineWidth = static_cast<std::uint16_t>(LineW);
			}

			UI::Table::NextRow();
			UI::DragFloat("Polygon Alpha", CDebugRenderer::PolygonConf.Alpha, 0.01, 0.0f, 1.0f, "%.2f");
			UI::Table::NextRow();
			UI::DragFloat("Solid Polygon Alpha", CDebugRenderer::PolygonSolidConf.Alpha, 0.01, 0.0f, 1.0f, "%.2f");

			UI::Table::NextRow();
			static EColor TransformColor = EColor::White;
			const bool ColorDeduced = FColor::DeduceEnum(TransformColor, CDebugRenderer::TransformConf.Color);
			if (UI::ColorDropdown(TransformColor)) {
				CDebugRenderer::TransformConf.Color = FColor::Get(TransformColor);
			}

			/* Editor Camera */
			if (CEditorCamera* EC = GetEditorCamera()) {
				UI::Table::NextRow();
				bool LerpEnabled = EC->IsLerpEnabled();
				if (UI::Checkbox("Camera Lerp", LerpEnabled)) {
					EC->SetLerpEnabled(LerpEnabled);
					PendingEditorCameraLerp = LerpEnabled;
				}
			}

			UI::EndPropertyGrid();
			ImGui::TreePop();
		}
		if (!SceneActive) {
			ImGui::EndDisabled();
		}
		ImGui::EndChild();
		ImGui::SameLine(0.0f, 0.0f);

		ImGui::BeginChild("##PlayerInfo", ImVec2(std::max(Avail.x * 0.33f, 580.0f), Avail.y), ImGuiChildFlags_None);
		ImGui::Indent();
		UI_Player();
		ImGui::Unindent();
		ImGui::EndChild();
		ImGui::SameLine(0.0f, 0.0f);

		ImGui::BeginChild("##PlayerMod", ImVec2(std::max(Avail.x * 0.33f, 580.0f), Avail.y), ImGuiChildFlags_None);
		ImGui::Indent();
		{
			UI::FScopedStyleStack StyleStack(
				ImGuiStyleVar_FramePadding, ImVec2(8, 3),
				ImGuiStyleVar_FrameRounding, 4.0f);
			if (ImGui::Button("Kill Player")) {
				GetSystem<CHealthSystem>().Kill(*Player);
			}

			static float Damage = 25.0f;
			if (ImGui::Button("Damage Player")) {
				GetSystem<CHealthSystem>().ApplyDamage(*Player, Damage);
			}
			ImGui::SameLine(0, 6);
			ImGui::SetNextItemWidth(50.0f);
			UI::DragFloat("Damage", Damage, 1.0f, 1.0f, 100.0f, "%.0f");

			UI::SpriteSheetModification(Player);
		}
		ImGui::Unindent();
		ImGui::EndChild();

		UI::End();
	}
}
