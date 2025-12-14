#include "editor.h"

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
#include "game/gameplaysystem.h"
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

namespace platformer2d {

	namespace {
		/*************************************
		 *        GAME SPECIFICATION
		 *************************************/
		const FGameSpecification GameSpec = {
			.LevelName = "TestLevel",
			.LevelFilepath = std::filesystem::path(LEVELS_DIR "/testlevel.yaml"),
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
		glm::vec2 PLAYER_SPAWN = { 0.0f, 0.0f };
		glm::vec2 GRAVITY = { 0.0f, -5.0f };
		float SCENE_LOAD_CAMERA_ZOOM = 0.30f;

		/* @todo Remove from here. Just temporary */
		bool bDrawCircle = false;
		bool bDrawCircleFilled = false;
		bool bDrawLine = false;
		glm::vec3 P1 = { 0.30f, -0.40, 0.50f };
		glm::vec3 DebugRot = { 0.0f, 0.0f, 0.0f };
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
		CRenderer::SetClearColor(FColor::SkyBlue);
	}

	void CEditor::Initialize()
	{
		LK_DEBUG_TAG("Editor", "Initialize");
		LK_VERIFY(Player == nullptr);

		CPhysicsWorld::OnSensorBeginEvent.Add(this, &CEditor::OnSensorBeginEvent);
		CPhysicsWorld::OnSensorEndEvent.Add(this, &CEditor::OnSensorEndEvent);
		CPhysicsWorld::OnContactBeginEvent.Add(this, &CEditor::OnContactBeginEvent);
		CPhysicsWorld::OnContactEndEvent.Add(this, &CEditor::OnContactEndEvent);

		Deserialize(GameSpec.LevelFilepath);
		OpenScene();
		LastSceneFilepath = Scene->GetFilepath();
		LK_TRACE_TAG("Editor", "Last scene filepath: {}", LastSceneFilepath);

		CWindow::OnResized.Add(this, &CEditor::OnWindowResized);
		CWindow* Window = CWindow::Get();
		Window->Maximize();
		UpdateViewportBounds();

		CKeyboard::OnKeyPressed.Add(this, &CEditor::OnKeyPressed);
		CMouse::OnButtonPressed.Add(this, &CEditor::OnMouseButtonPressed);

		LK_DEBUG_TAG("Editor", "Initialize editor resources");
		EditorResources.Initialize();

		CScene::OnActorCreated.Add([&](const LUUID Handle, std::weak_ptr<CActor> ActorRef)
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

		CScene::OnActorDeleted.Add([&](const LUUID Handle)
		{
			if (!Scene) {
				return;
			}
			LK_DEBUG_TAG("Editor", "OnActorDeleted: {}", Handle);
			UpdateInputBuffer(Scene->GetActors().size());
			UI::Widget::OnActorDeleted(Handle);
		});

		UI::OnGameMenuOpened.Add([&](const bool Opened)
		{
			if (!Scene) {
				LK_TRACE_TAG("Editor", "Game menu toggled, no scene active");
				return;
			}

			if (Opened) {
				CPhysicsWorld::Pause();
			} else {
				CPhysicsWorld::Unpause();
			}
		});
	}

	void CEditor::Destroy()
	{
		LK_TRACE_TAG("Editor", "Destroy");
		Serialize(GameSpec.LevelFilepath);

		LK_DEBUG_TAG("Editor", "Release level resources");
		Player.reset();
		Player = nullptr;
		Scene.reset();
		Scene = nullptr;

		EditorResources.Destroy();
	}

	void CEditor::OnAttach()
	{
		LK_TRACE_TAG("Editor", "OnAttach");
		Initialize();
	}

	void CEditor::OnDetach()
	{
		LK_TRACE_TAG("Editor", "OnDetach");
		Destroy();
	}

	void CEditor::Tick(const float InDeltaTime)
	{
		DeltaTime = InDeltaTime;
		if (!Scene) {
			if (bOpenSceneNextTick) {
				OpenScene();
				bOpenSceneNextTick = false;
			}
			return;
		} else if (bCloseSceneNextTick) {
			CloseScene();
			bCloseSceneNextTick = false;
			return;
		}

		CCamera& Camera = Player->GetCamera();
		Camera.SetViewportSize(EditorViewportWidth, EditorViewportHeight);
		CRenderer::BeginScene(Camera);

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
			Player->GetOutlineColor()
		);

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

		Scene->Render();
	}

	void CEditor::RenderUI()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		if (!UI::Begin(UI::PanelID::CoreViewport, nullptr, UI::CoreViewportFlags)) {
			ImGui::PopStyleVar(2);
			return;
		}

		UI_PrepareEditorViewport();
		const bool EditorViewportOpen = UI::Begin(UI::PanelID::EditorViewport, nullptr, UI::EditorViewportFlags);
		ImGui::PopStyleVar(2);
		if (EditorViewportOpen) {
			UI_LeftSidebar();

			UpdateEditorViewportState();
			if (Scene) {
				UI_ViewportTexture();
			} else {
				UI_LevelLauncher();
			}

			UI_Level();

			if (Scene) {
				UI::Statistics();
				UI::PlayerHud(Player);
				UI::SelectionPanel();
				UI_DrawGizmo();
			}

			UI::End(); /* ~EditorViewport */
		}

		UI::End(); /* ~Viewport */
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
			MousePos.y
		);

		for (const auto& Actor : TargetScene->GetActors()) {
			const glm::vec2 Pos = Actor->GetPosition();
			const glm::vec2 Size = Actor->GetSize();
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

		if (HitResults.empty()) {
			return 0;
		}

		std::sort(HitResults.begin(), HitResults.end(), [](auto& Lhs, auto& Rhs) { return Lhs.Distance < Rhs.Distance; });
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

		std::sort(HitResults.begin(), HitResults.end(), [](const auto& Lhs, const auto& Rhs) { return Lhs.Distance < Rhs.Distance; });
		return static_cast<uint16_t>(HitResults.size());
	}

	bool CEditor::Serialize(const std::filesystem::path& OutFile) const
	{
		LK_INFO_TAG("Editor", "Serialize: {}", OutFile);
		YAML::Emitter Out;

		Out << YAML::BeginMap; /* Level */
		Out << YAML::Key << "Level" << YAML::Value << Name;

		LK_TRACE("LastSceneFilepath: {}", LastSceneFilepath);
		const std::filesystem::path ScenePath = (Scene ? Scene->GetFilepath() : LastSceneFilepath);
		LK_DEBUG_TAG("Editor", "Scene path: {}", ScenePath);
		Out << YAML::Key << "Scene" << YAML::Value << Core::GetRelativeFromProject(ScenePath);

		Out << YAML::Key << "PlayerSpawn" << YAML::Value << PLAYER_SPAWN;
		Out << YAML::Key << "CameraZoom" << YAML::Value << SCENE_LOAD_CAMERA_ZOOM;

		/* Physics */
		Out << YAML::Key << "Physics";
		Out << YAML::BeginMap;
		Out << YAML::Key << "Gravity" << YAML::Value << CPhysicsWorld::GetGravity();
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
		bEditorViewportHovered = (PosX >= EditorViewportBounds[0].x) &&
			(PosY >= EditorViewportBounds[0].y) &&
			(PosX <= EditorViewportBounds[1].x) &&
			(PosY <= EditorViewportBounds[1].y);
	}

	void CEditor::UpdateEditorViewportBounds()
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
			(EditorViewportHeight != static_cast<uint16_t>(VpHeight))) {
			EditorViewportWidth  = static_cast<uint16_t>(VpWidth);
			EditorViewportHeight = static_cast<uint16_t>(VpHeight);
			CRenderer::GetViewportFramebuffer()->Resize(EditorViewportWidth, EditorViewportHeight);
		}
	}

	void CEditor::UpdateViewportBounds()
	{
		ViewportBounds[0] = { 0.0f, 0.0f };
		if (CWindow* Window = CWindow::Get(); Window != nullptr) {
			ViewportBounds[1] = Window->GetSize();
		} else {
			LK_WARN_TAG("Editor", "Failed to update viewport bounds");
			ViewportBounds[1] = { 0.0f, 0.0f };
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
			((MouseY / static_cast<float>(VpHeight)) * 2.0f - 1.0f) * -1.0f
		);
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
		Player = std::make_shared<CPlayer>(Spec.Player.ActorSpec , Spec.Player.BodySpec);

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

		UI::Font::Push(EFont::SourceSansPro, EFontSize::Regular, EFontModifier::Normal);

		ImGui::Spacing();
		ImGui::Text("Last Scene Filepath: %s", std::filesystem::relative(LastSceneFilepath, PROJECT_DIR).generic_string().c_str());
		UI::SetTooltip(LastSceneFilepath.generic_string());

		ImGui::Text("Scene To Open: %s", std::filesystem::relative(SceneToOpen, PROJECT_DIR).generic_string().c_str());
		UI::SetTooltip(SceneToOpen.generic_string());

		ImGui::Text("Open Scene Next Tick: %s", bOpenSceneNextTick ? "Yes" : "No");

		ImGui::Spacing();
		ImGui::SeparatorText("Editor Viewport");
		ImGui::Text("Focused: %d", bEditorViewportFocused);
		ImGui::Text("Hovered: %d", bEditorViewportHovered);

		if (Scene) {
			ImGui::Spacing();
			UI::Widget::Vec2Control("Gravity", GRAVITY, 0.0f, 0.010f);
			UI::Widget::Vec2Control("Player Spawn", PLAYER_SPAWN, 0.0f, 0.010f);
			UI::HelpMarker("The applied camera zoom when a scene is loaded");
			ImGui::SameLine();
			UI::Widget::DragFloat("Initial Camera Zoom", SCENE_LOAD_CAMERA_ZOOM, 0.01f, 0.0f, 1.0f);

			ImGui::Spacing();
			ImGui::Checkbox("Draw Circle", &bDrawCircle);
			ImGui::SameLine();
			ImGui::Checkbox("Draw Circle Filled", &bDrawCircleFilled);
			ImGui::SameLine();
			ImGui::Checkbox("Draw Line", &bDrawLine);
			UI::Widget::Vec3Control("P0", P1, 0.0f, 0.010f);
			UI::Widget::DragFloat("Radius", DebugRadius, 0.010f, 0.0f, 10.0f);
		}

		/* @todo Move this to UI */
		if (Player) {
			if (std::shared_ptr<CRifle> Rifle = Player->GetRifle()) {
				ImGui::Dummy(ImVec2(0, 10));
				ImGui::Separator();
				UI::LargeTextCentralized("Rifle");
				ImGui::Spacing();

				ImGui::Spacing();
				float ProjectileRadius = Rifle->GetProjectileRadius();
				if (UI::Widget::DragFloat("Projectile Radius", ProjectileRadius, 0.0010f, 0.0010f, 1.0f)) {
					Rifle->SetProjectileRadius(ProjectileRadius);
				}

				ImGui::Spacing();
				float ProjectileVelocity = Rifle->GetProjectileVelocity();
				if (UI::Widget::DragFloat("Projectile Velocity", ProjectileVelocity, 0.10f, 0.0f, 20.0f)) {
					Rifle->SetProjectileVelocity(ProjectileVelocity);
				}

				ImGui::Spacing();
				bool ExplodeOnImpact = Rifle->GetProjectileExplodeOnImpact();
				if (ImGui::Checkbox("Explode On Impact", &ExplodeOnImpact)) {
					Rifle->SetProjectileExplodeOnImpact(ExplodeOnImpact);
				}

				ImGui::SameLine();
				bool ShootEnabled = Rifle->IsEnabled();
				if (ImGui::Checkbox("Shooting Enabled", &ShootEnabled)) {
					Rifle->SetEnabled(ShootEnabled);
				}

				ImGui::Spacing();
				EColor ProjectileColor = EColor::Red;
				const bool ColorDeduced = FColor::DeduceEnum(ProjectileColor, Rifle->GetProjectileColor());
				if (!ColorDeduced) {
					ImGui::BeginDisabled();
				}
				if (UI::ColorDropdown(ProjectileColor)) {
					Rifle->SetProjectileColor(FColor::Get(ProjectileColor));
				}
				if (!ColorDeduced) {
					ImGui::EndDisabled();
				}
			}
		}

		ImGui::Spacing();

		if (ImGui::Button("Close scene")) {
			bCloseSceneNextTick = true;
		}
		ImGui::SameLine(0, 10.0f);

		const bool HasSceneRef = HasScene();
		if (HasSceneRef) {
			ImGui::BeginDisabled();
		}
		if (ImGui::Button("Open scene")) {
			bOpenSceneNextTick = true;
		}
		if (HasSceneRef) {
			ImGui::EndDisabled();
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

			ImGui::TreePop();
		}

		ImGui::Dummy(ImVec2(0, 8));

		if (Scene) {
			/**
			 * Get vector copy of all actors to not invalidate the iteration
			 * if an actor is deleted.
			 */
			const auto Actors = Scene->GetActors();
			ImGui::Text("Actors: %d", Actors.size() + 1);
			UI::Widget::ActorNode(Player, Scene);
			for (auto& Actor : Actors) {
				UI::Widget::ActorNode(Actor, Scene);
			}

			ImGui::Dummy(ImVec2(0, 10));
			ImGui::Separator();
			ImGui::Dummy(ImVec2(0, 10));
			UI::CreatorMenu(Scene);
		}

		UI::Font::Pop();
		UI::End();
	}

	void CEditor::UI_Player()
	{
		UI::PlayerData(Player);
	}

	void CEditor::UI_ViewportTexture()
	{
		const ImVec2 WindowSize = {
			static_cast<float>(EditorViewportWidth),
			static_cast<float>(EditorViewportHeight)
		};

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

		if (ImGuiWindow* Window = ImGui::FindWindowByName(UI::PanelID::EditorViewport)) {
			ImGui::Begin(Window->Name, nullptr, UI::CoreViewportFlags | ImGuiWindowFlags_NoScrollbar);

			CCamera& Camera = Player->GetCamera();
			if (UI::DrawGizmo(Gizmo, *SelectedRef, Camera.GetViewMatrix(), Camera.GetProjectionMatrix())) {
				Player->SetAwake(true);
			}

			ImGui::End();
		}
	}

	void CEditor::UI_PrepareEditorViewport()
	{
		ImGuiWindow* Window = ImGui::FindWindowByName(UI::PanelID::EditorViewport);
		if (!Window) {
			return;
		}

		ImGuiDockNode* DockNode = Window->DockNode;
		if (!DockNode) {
			return;
		}

		if ((DockNode->Size.x <= 0.0f) || (DockNode->Size.y <= 0.0f)) {
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
			ImGuiStyleVar_FrameRounding, 12.0f
		);
		UI::FScopedFont ButtonFont(EFont::Roboto, EFontSize::Header, EFontModifier::Bold);

		static constexpr ImVec2 ButtonSize(260.0f, 68.0f);
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
	}

	void CEditor::OnWindowResized(const uint16_t InWidth, const uint16_t InHeight)
	{
		LK_TRACE_TAG("Editor", "Window resized: ({}, {})", ViewportWidth, ViewportHeight);
		ViewportWidth = InWidth;
		ViewportHeight = InHeight;
	}

	void CEditor::OnKeyPressed(const FKeyData& Data)
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

				switch (IC->GetType())
				{
					case EInteraction::Damage:
					{
						auto& Data = std::get<FDamageInteraction>(IC->GetData());
						LK_WARN("Damage={}", Data.Damage);
						break;
					}
					case EInteraction::Pickup:
					{
						auto& Data = std::get<FPickupInteraction>(IC->GetData());
						LK_WARN("Kind={} ExpireOnPickup={}", Enum::ToString(Data.Kind), Data.bExpireWhenPickedUp);
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

	void CEditor::OpenScene()
	{
		if (SceneToOpen.empty()) {
			LK_ERROR_TAG("Editor", "No scene to open");
			return;
		}
		if (Scene) {
			LK_ERROR_TAG("Editor", "A scene is already open");
			return;
		}

		CPhysicsWorld::Initialize(GRAVITY);

		Scene = std::make_shared<CScene>("Editor");
		Scene->Deserialize(SceneToOpen);
		CreatePlayer();
		LK_VERIFY(Player);
		CPhysicsWorld::SetPreSolve(PreSolve, Player.get());

		CCamera* Camera = GetActiveCamera();
		LK_VERIFY(Camera);
		Camera->SetZoom(SCENE_LOAD_CAMERA_ZOOM);

		std::shared_ptr<CFramebuffer> Framebuffer = CRenderer::GetViewportFramebuffer();
		Framebuffer->GetImage(0)->Invalidate();
		Framebuffer->Invalidate();
	}

	void CEditor::CloseScene()
	{
		if (!Scene) {
			LK_WARN_TAG("Editor", "Cannot close scene, none is active");
			return;
		}

		UI::CloseGameMenu();

		std::filesystem::path ScenePath = Scene->GetFilepath();
		LK_INFO_TAG("Editor", "Save scene: {}", ScenePath);
		LastSceneFilepath = ScenePath;
		if (SceneToOpen.empty()) {
			SceneToOpen = LastSceneFilepath;
		}
		Scene->Serialize(ScenePath);

		LK_TRACE_TAG("Editor", "Release current scene and player");
		Scene.reset();
		Scene = nullptr;
		Player.reset();
		Player = nullptr;

		CPhysicsWorld::Destroy();

		std::shared_ptr<CFramebuffer> Framebuffer = CRenderer::GetViewportFramebuffer();
		Framebuffer->GetImage(0)->Invalidate();
		Framebuffer->Invalidate();

		LK_DEBUG_TAG("Editor", "Scene closed");
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

		const b2Vec2 Up = { 0.0f, 1.0f };
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
