#include "instance.h"

#include <algorithm>

#include "core/profiler.h"
#include "core/selectioncontext.h"
#include "core/string.h"
#include "core/window.h"
#include "core/input/keyboard.h"
#include "core/input/mouse.h"
#include "checkpointsystem.h"
#include "combatsystem.h"
#include "enemy.h"
#include "gameplaysystem.h"
#include "healthsystem.h"
#include "inputsystem.h"
#include "interactionsystem.h"
#include "projectilesystem.h"
#include "controller/patrolcontroller.h"
#include "renderer/renderer.h"
#include "renderer/debugrenderer.h"
#include "renderer/ui/ui.h"
#include "physics/body.h"
#include "physics/ray.h"
#include "scene/scene.h"

namespace platformer2d {

	CGameInstance::CGameInstance(CGameInstance* InstanceRef, const FGameSpecification& InSpec)
		: CLayer(InSpec.InstanceName)
		, Spec(InSpec)
	{
		Instance = InstanceRef;
		LK_VERIFY(Instance, "Invalid game instance reference");

		UpdateViewportBounds();

		RegisterSystem<CInputSystem>();
		RegisterSystem<CHealthSystem>();
		RegisterSystem<CGameplaySystem>();
		RegisterSystem<CCheckpointSystem>();
		RegisterSystem<CInteractionSystem>();
		RegisterSystem<CProjectileSystem>();
		RegisterSystem<CCombatSystem>();
	}

	CGameInstance::~CGameInstance()
	{
		Instance = nullptr;
	}

	void CGameInstance::OnAttach()
	{
		LK_TRACE_TAG("GameInstance", "OnAttach: {}", Spec.InstanceName);
		Initialize();
	}

	void CGameInstance::OnDetach()
	{
		LK_TRACE_TAG("GameInstance", "OnDetach: {}", Spec.InstanceName);
		Destroy();
	}

	void CGameInstance::Initialize()
	{
		InitializeSystems();

		Deserialize(Spec.LevelFilepath);
		OpenScene(SceneToOpen);
		LK_VERIFY(Scene);
		LastSceneFilepath = Scene->GetFilepath();
		LK_TRACE_TAG("GameInstance", "Last scene filepath: {}", LastSceneFilepath);

		GetSystem<CCheckpointSystem>().LoadFromDisk(Spec.LevelFilepath);

		DelegateHandles.OnWindowResized = CWindow::OnResized.Add(this, &CGameInstance::OnWindowResized);
		CWindow::Get().Maximize();
		UpdateViewportBounds();

		LK_DEBUG_TAG("GameInstance", "Register delegates");
		DelegateHandles.OnKey = CKeyboard::OnKeyEvent.Add(this, &CGameInstance::OnKey);
		DelegateHandles.OnMouseButton = CMouse::OnButtonEvent.Add(this, &CGameInstance::OnMouseButton);
		DelegateHandles.OnActorCreated = CScene::OnActorCreated.Add(this, &CGameInstance::OnActorCreated);
		DelegateHandles.OnActorDeleted = CScene::OnActorDeleted.Add(this, &CGameInstance::OnActorDeleted);
		DelegateHandles.OnPauseMenuOpened = UI::OnPauseMenuOpened.Add(this, &CGameInstance::OnPauseMenuToggled);

		const auto EnemyActors = Scene->GetAllOfType<CEnemy>();
		for (const auto& EnemyActor : EnemyActors) {
			IEnemyController* Controller = EnemyActor->GetController();
			if (!Controller) {
				continue;
			}
			if (Controller->GetControllerType() == EControllerType::Patrol) {
				static_cast<CPatrolController*>(Controller)->SetTarget(GetPlayer(0));
			}
		}

		OnInitialize();
	}

	void CGameInstance::Destroy()
	{
		LK_TRACE_TAG("GameInstance", "Destroy: {}", Spec.InstanceName);

		CWindow::OnResized.Remove(DelegateHandles.OnWindowResized);
		CKeyboard::OnKeyEvent.Remove(DelegateHandles.OnKey);
		CMouse::OnButtonEvent.Remove(DelegateHandles.OnMouseButton);
		CScene::OnActorCreated.Remove(DelegateHandles.OnActorCreated);
		CScene::OnActorDeleted.Remove(DelegateHandles.OnActorDeleted);
		UI::OnPauseMenuOpened.Remove(DelegateHandles.OnPauseMenuOpened);
		ShutdownSystems();

		Serialize(Spec.LevelFilepath);
		CloseScene();

		Player.reset();
		Scene.reset();

		OnShutdown();

		LK_VERIFY(!CPhysicsWorld::IsValid(), "Physics world still active");
	}

	void CGameInstance::Tick(const float InDeltaTime)
	{
		LK_PROFILE_FUNC();
		OnPreTick(InDeltaTime);

		const ESceneState SceneState = Scene ? Scene->GetState() : ESceneState::None;
		DeltaTime = (SceneState == ESceneState::Play) ? InDeltaTime : 0.0f;

		if (!Scene) {
			if (bOpenSceneNextTick) {
				OpenScene(SceneToOpen);
				bOpenSceneNextTick = false;
			}
			return;
		}
		if (bCloseSceneNextTick) {
			CloseScene();
			bCloseSceneNextTick = false;
			return;
		}

		CCamera* ActiveCamera = GetActiveCamera();
		LK_VERIFY(ActiveCamera);
		const auto [VpWidth, VpHeight] = GetActiveViewportSize();
		ActiveCamera->SetViewportSize(VpWidth, VpHeight);
		CRenderer::BeginScene(*ActiveCamera);

		Player->Tick(DeltaTime);
		Scene->Tick(DeltaTime);

		if (bRaycastScene) {
			RaycastSceneAtMouse();
		}

		CScene::RenderActor(*Player);

		TickSystems();
		OnPostTick(InDeltaTime);

		Scene->Render();
	}

	CCamera* CGameInstance::GetActiveCamera() const
	{
		return (Player ? &Player->GetCamera() : nullptr);
	}

	std::shared_ptr<CPlayer> CGameInstance::GetPlayer(const std::size_t Idx) const
	{
		LK_ASSERT(Idx == 0, "Only 1 player supported (for now)");
		return Player;
	}

	void CGameInstance::PauseGame()
	{
		LK_DEBUG_TAG("GameInstance", "Game paused");
		CPhysicsWorld::Pause();
		if (Scene) {
			Scene->SetState(ESceneState::Pause);
		}
	}

	void CGameInstance::ResumeGame()
	{
		LK_DEBUG_TAG("GameInstance", "Game resumed");
		CPhysicsWorld::Unpause();
		if (Scene) {
			Scene->SetState(ESceneState::Play);
		}
	}

	bool CGameInstance::IsGamePaused()
	{
		return (Scene ? (Scene->GetState() == ESceneState::Pause) : true);
	}

	std::uint16_t CGameInstance::RaycastScene(std::shared_ptr<CScene> TargetScene, std::vector<FHitResult>& HitResults)
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

		std::sort(HitResults.begin(), HitResults.end(), [](const auto& Lhs, const auto& Rhs)
		{
			return Lhs.Distance < Rhs.Distance;
		});
		return static_cast<std::uint16_t>(HitResults.size());
	}

	std::uint16_t CGameInstance::PickSceneAtMouse(std::shared_ptr<CScene> TargetScene, std::vector<FHitResult>& HitResults)
	{
		LK_UNUSED(TargetScene, HitResults);
		return 0;
	}

	void CGameInstance::OpenScene(const std::filesystem::path& ScenePath)
	{
		LK_INFO_TAG("GameInstance", R"(Open scene: "{}")", ScenePath);
		if (ScenePath.empty()) {
			LK_ERROR_TAG("GameInstance", "No scene to open");
			return;
		}
		if (Scene) {
			LK_ERROR_TAG("GameInstance", "A scene is already open");
			return;
		}

		CPhysicsWorld::Initialize(LevelData.Gravity);

		Scene = std::make_shared<CScene>(Spec.InstanceName);
		Scene->Deserialize(ScenePath);
		Scene->SetState(ESceneState::Play);
		CreatePlayer();
		LK_VERIFY(Player);
		CPhysicsWorld::SetPreSolve(&CGameInstance::PreSolve, Player.get());

		OnSceneOpened();

		std::shared_ptr<CFramebuffer> Framebuffer = CRenderer::GetViewportFramebuffer();
		Framebuffer->GetImage(0)->Invalidate();
		Framebuffer->Invalidate();

		CWindow::Get().SetTitle(Format("platformer2d - {} - {} ({})", Spec.InstanceName, Scene->GetName(), Core::GetPlatformName()));
		SceneToOpen.clear();
	}

	void CGameInstance::CloseScene()
	{
		if (!Scene) {
			LK_WARN_TAG("GameInstance", "Cannot close scene, none is active");
			return;
		}

		OnSceneClosing();

		UI::ClosePauseMenu();

		LK_TRACE_TAG("GameInstance", "Release current scene and player");
		Scene.reset();
		Player.reset();

		LevelData.CachedGravity = CPhysicsWorld::GetGravity();
		CPhysicsWorld::Destroy();

		std::shared_ptr<CFramebuffer> Framebuffer = CRenderer::GetViewportFramebuffer();
		Framebuffer->GetImage(0)->Invalidate();
		Framebuffer->Invalidate();

		CWindow::Get().SetTitle(Format("platformer2d ({})", Core::GetPlatformName()));
		LK_DEBUG_TAG("GameInstance", "Scene closed");
	}

	void CGameInstance::CreatePlayer()
	{
		LK_DEBUG_TAG("GameInstance", "Create player: JumpImpulse={} DirForce={}", Spec.Player.BodySpec.JumpImpulse, Spec.Player.BodySpec.DirForce);
		Player = std::make_shared<CPlayer>(Spec.Player.ActorSpec, Spec.Player.BodySpec);

		Player->OnJumped.Add([](const FPlayerData& PlayerData)
		{
			LK_TRACE("Player {} jumped", PlayerData.ID);
		});

		Player->OnLanded.Add([](const FPlayerData& PlayerData)
		{
			LK_TRACE("Player {} landed", PlayerData.ID);
		});

		Player->OnDied.Add([this](const FPlayerData& PlayerData)
		{
			auto& Checkpoint = GetSystem<CCheckpointSystem>();
			LK_INFO_TAG("GameInstance", "Player {} died, respawning at {}", PlayerData.ID, Checkpoint.HasCheckpoint() ? "checkpoint" : "at default spawn");
			if (Checkpoint.HasCheckpoint()) {
				Checkpoint.RestoreToPlayer(*Player);
			} else {
				LK_DEBUG_TAG("GameInstance", "No checkpoint");
				auto& HC = Player->GetComponent<FHealthComponent>();
				HC.SetMaxHealth();

				CBody* Body = Player->GetBody();
				LK_ASSERT(Body);
				Body->SetEnabled(true);
				Body->SetLinearVelocity({0.0f, 0.0f});

				/* Find any spawnpoints. */
				std::vector<std::shared_ptr<CActor>> Spawnpoints;
				if (Scene->GetAllWithFlags(EActorFlag_Spawnpoint, Spawnpoints) > 0) {
					GetSystem<CGameplaySystem>().Teleport(Player, Spawnpoints.at(0)->GetPosition());
				} else {
					GetSystem<CGameplaySystem>().Teleport(Player, LevelData.PlayerSpawn);
				}
			}
		});

		GetSystem<CGameplaySystem>().Teleport(Player, LevelData.PlayerSpawn);
		Player->GetCamera().SetZoom(LevelData.SceneLoadCameraZoom);

		OnPlayerCreated();
	}

	void CGameInstance::SaveScene()
	{
		if (!Scene) {
			LK_WARN_TAG("GameInstance", "Cannot save scene, none is active");
			return;
		}

		const std::filesystem::path ScenePath = Scene->GetFilepath();
		LK_INFO_TAG("GameInstance", "Save scene: {}", ScenePath);
		LastSceneFilepath = ScenePath;
		if (SceneToOpen.empty()) {
			SceneToOpen = LastSceneFilepath;
		}
		Scene->Serialize(ScenePath);
	}

	void CGameInstance::MousePickScene()
	{
		CCamera* Camera = GetActiveCamera();
		if (!Scene || !Camera) {
			return;
		}

		static std::vector<FHitResult> HitResults;
		const std::uint16_t Picked = PickSceneAtMouse(Scene, HitResults);
		if (Picked > 0) {
			const FHitResult& Hit = HitResults.at(0);
			if (std::shared_ptr<CActor> Ref = Hit.Ref.lock(); Ref != nullptr) {
				CSelectionContext::Select(Ref->GetHandle());
			}
		}
	}

	void CGameInstance::RaycastSceneAtMouse()
	{
		CCamera* Camera = GetActiveCamera();
		if (!Scene || !Camera) {
			return;
		}

		static std::vector<FHitResult> HitResults;
		const std::uint16_t Hits = RaycastScene(Scene, HitResults);
		LK_UNUSED(Hits);
	}

	void CGameInstance::OnKey(const FKeyData& Data)
	{
		switch (Data.Key) {
			case EKey::Escape:
				if (Data.State == EKeyState::Pressed) {
					UI::TogglePauseMenu();
				}
				break;
			default:
				break;
		}

		if (Player) {
			Player->OnKey(Data);
		}
	}

	void CGameInstance::OnMouseButton(const FMouseButtonData& Data)
	{
		LK_TRACE_TAG("GameInstance", "Button={} State={}", Enum::ToString(Data.Button), Enum::ToString(Data.State));
		switch (Data.Button) {
			case EMouseButton::Button0:
				if (Data.State == EMouseButtonState::Pressed) {
					MousePickScene();
				}
				break;
			default:
				break;
		}

		if (Player) {
			Player->OnMouseButton(Data);
		}
	}

	void CGameInstance::OnMouseScroll(const EMouseScrollDirection Direction)
	{
		if (Player) {
			Player->OnMouseScroll(Direction);
		}
	}

	void CGameInstance::OnWindowResized(const std::uint16_t InWidth, const std::uint16_t InHeight)
	{
		LK_TRACE_TAG("GameInstance", "Window resized: ({}, {})", InWidth, InHeight);
		ViewportWidth = InWidth;
		ViewportHeight = InHeight;

		if (Player) {
			Player->OnWindowResized(InWidth, InHeight);
		}
	}

	void CGameInstance::OnPauseMenuToggled(const bool Opened)
	{
		LK_TRACE_TAG("GameInstance", "Pause menu toggled, opened={}", Opened);
		if (!Scene) {
			return;
		}
		if (Opened) {
			PauseGame();
		} else {
			ResumeGame();
		}
	}

	std::pair<std::uint16_t, std::uint16_t> CGameInstance::GetActiveViewportSize() const
	{
		return {ViewportWidth, ViewportHeight};
	}

	glm::vec2 CGameInstance::GetMouseInViewportSpace()
	{
		glm::vec2 MousePos = CMouse::GetPos();
		MousePos.x -= ViewportBounds[0].x;
		MousePos.y -= ViewportBounds[0].y;
		const float VpWidth = ViewportBounds[1].x - ViewportBounds[0].x;
		const float VpHeight = ViewportBounds[1].y - ViewportBounds[0].y;
		return glm::vec2(
			(MousePos.x / static_cast<float>(VpWidth)) * 2.0f - 1.0f,
			((MousePos.y / static_cast<float>(VpHeight)) * 2.0f - 1.0f) * -1.0f);
	}

	glm::vec2 CGameInstance::GetMouseInWorldSpace(const CCamera& Camera)
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

	void CGameInstance::InitializeSystems()
	{
		for (std::unique_ptr<IGameSystem>& System : Systems) {
			if (System) {
				System->Initialize(*this);
			}
		}
	}

	void CGameInstance::ShutdownSystems()
	{
		for (std::size_t Idx = Systems.size(); Idx > 0; Idx--) {
			if (Systems[Idx - 1]) {
				Systems[Idx - 1]->Shutdown();
			}
		}
	}

	void CGameInstance::TickSystems()
	{
		for (std::size_t Idx = Systems.size(); Idx > 0; Idx--) {
			if (Systems[Idx - 1]) {
				Systems[Idx - 1]->Tick();
			}
		}
	}

	void CGameInstance::UpdateViewportBounds()
	{
		ViewportBounds[0] = {0.0f, 0.0f};
		ViewportBounds[1] = CWindow::Get().GetSize();
	}

	bool CGameInstance::PreSolve(b2ShapeId ShapeA, b2ShapeId ShapeB, b2Vec2 Point, b2Vec2 Normal, void* Ctx)
	{
		LK_ASSERT(b2Shape_IsValid(ShapeA) && b2Shape_IsValid(ShapeB));
		if (!Ctx) {
			return false;
		}

		CPlayer& Player = *static_cast<CPlayer*>(Ctx);
		const b2ShapeId PlayerShapeID = Player.GetBody()->GetShapeID();

		const bool InvolvesPlayer = B2_ID_EQUALS(ShapeA, PlayerShapeID) || B2_ID_EQUALS(ShapeB, PlayerShapeID);
		if (!InvolvesPlayer) {
			return true;
		}

		if (B2_ID_EQUALS(ShapeA, PlayerShapeID)) {
			Normal.x = -Normal.x;
			Normal.y = -Normal.y;
		}

		const b2Vec2 Up = {0.0f, 1.0f};
		const float UpDot = Normal.x * Up.x + Normal.y * Up.y;
		if (UpDot <= 0.0f) {
			return true;
		}

		const b2BodyId PlayerBody = Player.GetBody()->GetID();
		const b2Vec2 V = b2Body_GetLinearVelocity(PlayerBody);
		const float Vn = V.x * Normal.x + V.y * Normal.y;
		if (Vn > 0.0f) {
			return false;
		}

		return true;
	}

}

