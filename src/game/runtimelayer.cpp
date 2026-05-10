#include "runtimelayer.h"

#include <fstream>
#include <istream>
#include <numeric>

#include "core/window.h"
#include "core/profiler.h"
#include "core/timer.h"
#include "core/selectioncontext.h"
#include "core/string.h"
#include "core/input/keyboard.h"
#include "core/input/mouse.h"
#include "core/math/math.h"
#include "game/gameplaysystem.h"
#include "game/healthsystem.h"
#include "game/enemy.h"
#include "game/player.h"
#include "game/spawner.h"
#include "game/controller/patrolcontroller.h"
#include "renderer/renderer.h"
#include "renderer/debugrenderer.h"
#include "renderer/ui/ui.h"
#include "renderer/ui/widgets.h"
#include "renderer/ui/selectionpanel.h"
#include "physics/body.h"
#include "physics/physicsworld.h"
#include "physics/ray.h"
#include "serialization/serialization.h"

namespace platformer2d {

	namespace {
		const FGameSpecification GameSpec = {
			/* clang-format off */
			.InstanceName = "Runtime",
			.LevelFilepath = std::filesystem::path(LEVELS_DIR "/testlevel.yaml"),
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
	}

	bool PreSolve(b2ShapeId ShapeA, b2ShapeId ShapeB, b2Vec2 Point, b2Vec2 Normal, void* Ctx);

	CRuntimeLayer::CRuntimeLayer()
		: CGameInstance(this, GameSpec)
	{
		LK_TRACE_TAG("RuntimeLayer", "Instance created");
	}

	CRuntimeLayer::~CRuntimeLayer()
	{
		LK_TRACE_TAG("RuntimeLayer", "Destructor");
	}

	void CRuntimeLayer::Initialize()
	{
		DelegateHandles.OnSensorBeginEvent = CPhysicsWorld::OnSensorBeginEvent.Add(this, &CRuntimeLayer::OnSensorBeginEvent);
		DelegateHandles.OnSensorEndEvent = CPhysicsWorld::OnSensorEndEvent.Add(this, &CRuntimeLayer::OnSensorEndEvent);
		DelegateHandles.OnContactBeginEvent = CPhysicsWorld::OnContactBeginEvent.Add(this, &CRuntimeLayer::OnContactBeginEvent);
		DelegateHandles.OnContactEndEvent = CPhysicsWorld::OnContactEndEvent.Add(this, &CRuntimeLayer::OnContactEndEvent);

		Deserialize(GameSpec.LevelFilepath);
		OpenScene(SceneToOpen);
		LastSceneFilepath = Scene->GetFilepath();
		LK_TRACE_TAG("RuntimeLayer", "Last scene filepath: {}", LastSceneFilepath);

		CWindow::OnResized.Add(this, &CRuntimeLayer::OnWindowResized);
		CWindow& Window = CWindow::Get();
		Window.Maximize();
		UpdateViewportBounds();

		DelegateHandles.OnKeyPressed = CKeyboard::OnKeyPressed.Add(this, &CRuntimeLayer::OnKeyPressed);
		DelegateHandles.OnMouseButtonPressed = CMouse::OnButtonPressed.Add(this, &CRuntimeLayer::OnMouseButtonPressed);

		DelegateHandles.OnActorCreated = CScene::OnActorCreated.Add([&](const LUUID Handle, std::weak_ptr<CActor> ActorRef)
		{
			LK_TRACE_TAG("RuntimeLayer", "OnActorCreated: {}", Handle);
		});

		DelegateHandles.OnActorDeleted = CScene::OnActorDeleted.Add([&](const LUUID Handle)
		{
			LK_TRACE_TAG("RuntimeLayer", "OnActorDeleted: {}", Handle);
		});

		DelegateHandles.OnPauseMenuOpened = UI::OnPauseMenuOpened.Add([&](const bool Opened)
		{
			if (!Scene) {
				LK_TRACE_TAG("RuntimeLayer", "Pause menu toggled, no scene active");
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

	void CRuntimeLayer::Destroy()
	{
		LK_TRACE_TAG("RuntimeLayer", "Destroy");
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

		LK_DEBUG_TAG("RuntimeLayer", "Release level resources");
		Player.reset();
		Player = nullptr;
		Scene.reset();
		Scene = nullptr;

		LK_VERIFY(!CPhysicsWorld::IsValid(), "Physics world still active");
	}

	void CRuntimeLayer::OnAttach()
	{
		LK_TRACE_TAG("RuntimeLayer", "OnAttach");
		Initialize();
	}

	void CRuntimeLayer::OnDetach()
	{
		LK_TRACE_TAG("RuntimeLayer", "OnDetach");
		Destroy();
	}

	void CRuntimeLayer::Tick(const float InDeltaTime)
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
			HandleUpdatedSceneState(SceneState);
			bSceneStateChanged = false;
		}

		CCamera& Camera = Player->GetCamera();
		Camera.SetViewportSize(ViewportWidth, ViewportHeight);
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
			Player->GetOutlineColor());

		Scene->Render();
	}

	void CRuntimeLayer::RenderUI()
	{
		if (!UI::BeginViewport()) {
			return;
		}

		const ImVec2 WindowSize = ImGui::GetWindowSize();
		ViewportWidth = WindowSize.x;
		ViewportHeight = WindowSize.y;
		UI_ViewportTexture();

		UI::EndViewport();
	}

	CCamera* CRuntimeLayer::GetActiveCamera() const
	{
		return (Player ? &Player->GetCamera() : nullptr);
	}

	std::shared_ptr<CPlayer> CRuntimeLayer::GetPlayer(std::size_t Idx) const
	{
		LK_ASSERT(Idx == 0, "Only 1 player supported (for now)");
		return Player;
	}

	void CRuntimeLayer::PauseGame()
	{
		LK_DEBUG_TAG("RuntimeLayer", "Game paused");
		CPhysicsWorld::Pause();
		if (Scene) {
			Scene->SetState(ESceneState::Pause);
		}
	}

	void CRuntimeLayer::ResumeGame()
	{
		LK_DEBUG_TAG("RuntimeLayer", "Game resumed");
		CPhysicsWorld::Unpause();
		if (Scene) {
			Scene->SetState(ESceneState::Play);
		}
	}

	bool CRuntimeLayer::IsGamePaused()
	{
		return (Scene ? (Scene->GetState() == ESceneState::Pause) : true);
	}

	uint16_t CRuntimeLayer::RaycastScene(std::shared_ptr<CScene> TargetScene, std::vector<FHitResult>& HitResults)
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

	uint16_t CRuntimeLayer::PickSceneAtMouse(std::shared_ptr<CScene> TargetScene, std::vector<FHitResult>& HitResults)
	{
		return 0;
	}

	void CRuntimeLayer::OnSensorBeginEvent(const CSensorBeginEvent& Event)
	{
		LK_ASSERT(Event.Sensor && Event.Visitor);
		LK_DEBUG_TAG("RuntimeLayer", "OnSensorBeginEvent: Sensor={} Visitor={}", Event.Sensor->GetName(), Event.Visitor->GetName());
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

				std::visit([&](auto&& Data)
				{
					using T = std::decay_t<decltype(Data)>;
					if constexpr (std::is_same_v<T, FDamageInteraction>) {
						CHealthSystem::ApplyDamage(Event.Visitor, Data.Damage);
					} else if constexpr (std::is_same_v<T, FPickupInteraction>) {
						CPlayer& PlayerRef = *static_cast<CPlayer*>(Event.Visitor);
						OnPickupEvent(PlayerRef, *IC);
					} else if constexpr (std::is_same_v<T, FHealInteraction>) {
						CHealthSystem::Heal(Event.Visitor, Data.Amount);
						if (Data.bConsumeOnUse) {
							LK_DEBUG_TAG("RuntimeLayer", "[TODO] Despawn heal source {}", Event.Sensor->GetName());
						}
					} else if constexpr (std::is_same_v<T, FKillzoneInteraction>) {
						CHealthSystem::Kill(Event.Visitor);
					} else if constexpr (std::is_same_v<T, FJumppadInteraction>) {
						if (CBody* B = Event.Visitor->GetBody()) {
							const glm::vec2 Vel = B->GetLinearVelocity();
							const float NewX = Data.bPreserveHorizontalVelocity ? Vel.x : 0.0f;
							B->SetLinearVelocity({NewX, Data.Impulse.y});
						}
					} else if constexpr (std::is_same_v<T, FClimbableInteraction>) {
						static_cast<CPlayer*>(Event.Visitor)->SetClimbZone(true, Data.ClimbSpeed);
					}
				}, IC->GetData());
			}
		}
	}

	void CRuntimeLayer::OnSensorEndEvent(const CSensorEndEvent& Event)
	{
		LK_ASSERT(Event.Sensor && Event.Visitor);
		LK_DEBUG_TAG("RuntimeLayer", "OnSensorEndEvent: Sensor={} Visitor={}", Event.Sensor->GetName(), Event.Visitor->GetName());
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

				if (IC->GetType() == EInteraction::Climbable) {
					static_cast<CPlayer*>(Event.Visitor)->SetClimbZone(false);
				}
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

		CHealthSystem::ApplyDamage(HitActor, Projectile->GetDamage());

		if (Projectile->ExplodesOnImpact()) {
			Projectile->Destroy();
		} else if (Projectile->BounceCount >= Projectile->MaxBounceCount) {
			LK_TRACE("{}: Max bounce reached: {}", Projectile->GetName(), Projectile->BounceCount);
			Projectile->Destroy();
		}
	};

	void CRuntimeLayer::OnContactBeginEvent(const CContactBeginEvent& Event)
	{
		LK_TRACE_TAG("RuntimeLayer", "OnContactBeginEvent: A={} B={}", (Event.A ? Event.A->GetName() : "NULL"), (Event.B ? Event.B->GetName() : "NULL"));
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

	void CRuntimeLayer::OnContactEndEvent(const CContactEndEvent& Event)
	{
		LK_TRACE_TAG("RuntimeLayer", "OnContactEndEvent: A={} B={}", (Event.A ? Event.A->GetName() : "NULL"), (Event.B ? Event.B->GetName() : "NULL"));
		LK_ASSERT(Event.A && Event.B, "Invalid event references");
		if (!Event.A || !Event.B) {
			return;
		}
	}

	bool CRuntimeLayer::Serialize(const std::filesystem::path& OutFile) const
	{
		/** @todo: Player-specific data about checkpoints and progress should only be persistent. */
		LK_WARN_TAG("RuntimeLayer", "[TODO] Serialize: {}", OutFile);
		return true;
	}

	bool CRuntimeLayer::Deserialize(const std::filesystem::path& InFile)
	{
		LK_INFO_TAG("RuntimeLayer", "Deserialize: {}", StringUtils::GetPathRelativeToProject(InFile));
		LK_ASSERT(std::filesystem::exists(InFile), "Filepath does not exist: {}", InFile);
		if (!std::filesystem::exists(InFile)) {
			LK_ERROR_TAG("RuntimeLayer", "Filepath does not exist: {}", InFile);
			return false;
		}

		std::ifstream InputStream(InFile);
		std::stringstream StringStream;
		StringStream << InputStream.rdbuf();
		const std::string YamlString = StringStream.str();

		const YAML::Node Data = YAML::Load(YamlString);
		Serialization::DeserializeProperty("Gravity", LevelData.Gravity, glm::vec2(0.0f, -5.0f), Data);
		Serialization::DeserializeProperty("PlayerSpawn", LevelData.PlayerSpawn, glm::vec2(0.0f, 0.0f), Data);
		Serialization::DeserializeProperty("CameraZoom", LevelData.SceneLoadCameraZoom, 0.40f, Data);

		/* Load the scene. */
		const YAML::Node& SceneNode = Data["Scene"];
		LK_ASSERT(!SceneNode.IsNull());
		if (SceneNode.IsNull()) {
			LK_ERROR_TAG("RuntimeLayer", "Scene node is missing in YAML");
			return false;
		}

		const std::filesystem::path SceneFilepath = SceneNode.as<std::filesystem::path>();
		LK_INFO_TAG("RuntimeLayer", "Scene to open: {}", StringUtils::GetPathRelativeToProject(SceneFilepath));
		SceneToOpen = SceneFilepath;

		return !SceneToOpen.empty();
	}

	void CRuntimeLayer::OnWindowResized(uint16_t InWidth, uint16_t InHeight)
	{
		LK_TRACE_TAG("RuntimeLayer", "Window resized: ({}, {})", InWidth, InHeight);
		ViewportWidth = InWidth;
		ViewportHeight = InHeight;
	}

	void CRuntimeLayer::OnKeyPressed(const FKeyData& Data)
	{
		switch (Data.Key) {
			case EKey::Escape:
				if (Data.State == EKeyState::Pressed) {
					UI::TogglePauseMenu();
				}
				break;
		}
	}

	void CRuntimeLayer::OnMouseButtonPressed(const FMouseButtonData& Data)
	{
		switch (Data.State) {
			case EMouseButtonState::Pressed:
				if (Data.Button == EMouseButton::Button0) {
					MousePickScene();
				}
				break;
			case EMouseButtonState::Released:
				break;
			case EMouseButtonState::Held:
				break;
		}
	}

	void CRuntimeLayer::UI_ViewportTexture()
	{
		const ImVec2 WindowSize = {static_cast<float>(ViewportWidth), static_cast<float>(ViewportHeight)};
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

	void CRuntimeLayer::MousePickScene()
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
			}
		}
	}

	void CRuntimeLayer::RaycastScene()
	{
		CCamera* Camera = GetActiveCamera();
		if (!Scene || !Camera) {
			return;
		}

		static std::vector<FHitResult> HitResults;
		const uint16_t Hits = RaycastScene(Scene, HitResults);
		LK_UNUSED(Hits);
	}

	void CRuntimeLayer::OpenScene(const std::filesystem::path& ScenePath)
	{
		if (ScenePath.empty()) {
			LK_ERROR_TAG("RuntimeLayer", "No scene to open");
			return;
		}
		if (Scene) {
			LK_ERROR_TAG("RuntimeLayer", "A scene is already open");
			return;
		}

		CPhysicsWorld::Initialize(LevelData.Gravity);

		Scene = std::make_shared<CScene>("Runtime");
		Scene->Deserialize(ScenePath);
		Scene->SetState(ESceneState::Play);
		CreatePlayer();
		LK_VERIFY(Player);
		CPhysicsWorld::SetPreSolve(PreSolve, Player.get());

		CCamera* Camera = GetActiveCamera();
		LK_VERIFY(Camera);
		Camera->SetZoom(LevelData.SceneLoadCameraZoom);

		std::shared_ptr<CFramebuffer> Framebuffer = CRenderer::GetViewportFramebuffer();
		Framebuffer->GetImage(0)->Invalidate();
		Framebuffer->Invalidate();

		CWindow::Get().SetTitle(Format("platformer2d - {} ({})", Scene->GetName(), Core::GetPlatformName()));
		SceneToOpen.clear();
	}

	void CRuntimeLayer::CloseScene()
	{
		if (!Scene) {
			LK_WARN_TAG("RuntimeLayer", "Cannot close scene, none is active");
			return;
		}

		UI::ClosePauseMenu();

		/**
		 * @fixme: Need to make sure the scene is transient for runtime.
		 * The call to SaveScene might be redundant.
		 */
#if 0
		SaveScene();
#endif

		LK_TRACE_TAG("RuntimeLayer", "Release current scene and player");
		Scene.reset();
		Scene = nullptr;
		Player.reset();
		Player = nullptr;

		CPhysicsWorld::Destroy();

		std::shared_ptr<CFramebuffer> Framebuffer = CRenderer::GetViewportFramebuffer();
		Framebuffer->GetImage(0)->Invalidate();
		Framebuffer->Invalidate();

		CWindow::Get().SetTitle(Format("platformer2d ({})", Core::GetPlatformName()));
		LK_DEBUG_TAG("RuntimeLayer", "Scene closed");
	}

	void CRuntimeLayer::SaveScene()
	{
		if (!Scene) {
			LK_WARN_TAG("RuntimeLayer", "Cannot save scene, none is active");
			return;
		}

		std::filesystem::path ScenePath = Scene->GetFilepath();
		LK_INFO_TAG("RuntimeLayer", "Save scene: {}", ScenePath);
		LastSceneFilepath = ScenePath;
		if (SceneToOpen.empty()) {
			SceneToOpen = LastSceneFilepath;
		}
		Scene->Serialize(ScenePath);
	}

	void CRuntimeLayer::HandleUpdatedSceneState(const ESceneState NewState)
	{
		LK_DEBUG_TAG("RuntimeLayer", "New scene state: {}", Enum::ToString(NewState));
		if (NewState == ESceneState::Play) {
			CPhysicsWorld::Unpause();
		} else {
			CPhysicsWorld::Pause();
		}
	}

	void CRuntimeLayer::CreatePlayer()
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

		CGameplaySystem::Teleport(Player, LevelData.PlayerSpawn);
	}

	void CRuntimeLayer::OnPickupEvent(CPlayer& InPlayer, const FInteractionComponent& IC)
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

	void CRuntimeLayer::OnPickupEvent_Item(const FPickupInteraction& Interaction, CPlayer& InPlayer)
	{
		const auto& Object = std::get<FPickupItem>(Interaction.Object);
		LK_WARN("Item={} ExpireOnPickup={}", Enum::ToString(Object.Type), Interaction.bExpireWhenPickedUp);
	}

	void CRuntimeLayer::OnPickupEvent_Rifle(const FPickupInteraction& Interaction, CPlayer& InPlayer)
	{
		const auto& Object = std::get<FPickupWeapon>(Interaction.Object);
		const auto& Spec = std::get<FRifleSpecification>(Object.Spec);
		LK_TRACE("Pickup Weapon={} MagazineSize={} ExpireOnPickup={}", Enum::ToString(Object.Type), Spec.MagazineSize, Interaction.bExpireWhenPickedUp);
		CInventory& Inventory = InPlayer.GetInventory();
		if (Inventory.IsEmpty()) {
			std::shared_ptr<CRifle> Rifle = std::make_shared<CRifle>(Spec, &InPlayer);
			Inventory.AddItem(Rifle);
		} else {
			LK_WARN_TAG("RuntimeLayer", "Inventory not empty");
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
