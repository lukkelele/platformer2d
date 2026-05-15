#pragma once

#include <atomic>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "core/core.h"
#include "core/layer.h"
#include "gamesystem.h"
#include "player.h"
#include "physics/events.h"
#include "physics/physicsworld.h"
#include "physics/ray.h"

namespace platformer2d {

	class CScene;

	namespace Internal {
		inline std::size_t NextSystemID()
		{
			static std::atomic<std::size_t> Counter{0};
			return Counter.fetch_add(1, std::memory_order_relaxed);
		}

		template<typename T>
		inline std::size_t SystemID()
		{
			static const std::size_t ID = NextSystemID();
			return ID;
		}
	}

	struct FConfig
	{
		struct
		{
			bool bDrawRayHits = true;
		} Debug;
	};

	struct FGameSpecification
	{
		std::string InstanceName;
		std::filesystem::path LevelFilepath;

		struct
		{
			FActorSpecification ActorSpec{};
			FBodySpecification BodySpec{};
		} Player{};
	};

	struct FLevelData
	{
		glm::vec2 Gravity = {0.0f, -5.0f};
		glm::vec2 PlayerSpawn = {0.0f, 0.0f};
		float SceneLoadCameraZoom = 0.30f;
		glm::vec2 CachedGravity = {0.0f, -5.0f};
	};

	class CGameInstance : public CLayer, public ISerializable<ESerializable::File>
	{
	public:
		CGameInstance(CGameInstance* InstanceRef, const FGameSpecification& InSpec);
		CGameInstance() = delete;
		CGameInstance(CGameInstance&&) = delete;
		CGameInstance(const CGameInstance&) = delete;
		virtual ~CGameInstance();

		CGameInstance& operator=(CGameInstance&&) = delete;
		CGameInstance& operator=(const CGameInstance&) = delete;

		[[nodiscard]] static CGameInstance& Get() { return *Instance; }
		virtual void Initialize();
		virtual void Destroy();
		void OnAttach() override;
		void OnDetach() override;

		void Tick(float InDeltaTime) override;
		void RenderUI() override = 0;
		[[nodiscard]] float GetDeltaTime() const { return DeltaTime; }

		virtual bool Serialize(const std::filesystem::path& OutFile) const override = 0;
		virtual bool Deserialize(const std::filesystem::path& InFile) override = 0;

		template<typename T>
		[[nodiscard]] T* GetSystemPtr() const
		{
			const std::size_t ID = Internal::SystemID<T>();
			return (ID < Systems.size()) ? static_cast<T*>(Systems[ID].get()) : nullptr;
		}

		template<typename T>
		[[nodiscard]] T& GetSystem() const
		{
			T* Ptr = GetSystemPtr<T>();
			LK_ASSERT(Ptr, "System not registered");
			return *Ptr;
		}

		[[nodiscard]] static bool IsValid() { return (Instance != nullptr); }
		[[nodiscard]] virtual CCamera* GetActiveCamera() const;
		[[nodiscard]] std::shared_ptr<CPlayer> GetPlayer(std::size_t Idx = 0) const;
		[[nodiscard]] std::shared_ptr<CScene> GetScene() const { return Scene; }
		void OpenScene(const std::filesystem::path& ScenePath);
		void CloseScene();
		[[nodiscard]] bool HasScene() const { return Scene != nullptr; }

		void PauseGame();
		void ResumeGame();
		[[nodiscard]] bool IsGamePaused();

		[[nodiscard]] virtual glm::vec2 GetMouseInViewportSpace();
		[[nodiscard]] virtual glm::vec2 GetMouseInWorldSpace(const CCamera& Camera);
		[[nodiscard]] const std::filesystem::path& GetLastSceneFilepath() const { return LastSceneFilepath; }

	protected:
		virtual std::uint16_t RaycastScene(std::shared_ptr<CScene> TargetScene, std::vector<FHitResult>& HitResults);
		virtual std::uint16_t PickSceneAtMouse(std::shared_ptr<CScene> TargetScene, std::vector<FHitResult>& HitResults);

		template<typename T, typename... TArgs>
		T* RegisterSystem(TArgs&&... Args)
		{
			static_assert(std::is_base_of_v<IGameSystem, T>, "T must derive from IGameSystem");
			const std::size_t ID = Internal::SystemID<T>();
			if (Systems.size() <= ID) {
				Systems.resize(ID + 1);
			}
			LK_ASSERT(!Systems[ID], "System already registered");
			auto Owned = std::make_unique<T>(std::forward<TArgs>(Args)...);
			T* Raw = Owned.get();
			Systems[ID] = std::move(Owned);
			return Raw;
		}

		virtual void OnInitialize() {}
		virtual void OnShutdown() {}
		virtual void OnSceneOpened() {}
		virtual void OnSceneClosing() {}
		virtual void OnPlayerCreated() {}
		virtual void OnPreTick(float InDeltaTime) {}
		virtual void OnPostTick(float InDeltaTime) {}

		virtual void OnKey(const FKeyData& Data);
		virtual void OnMouseButton(const FMouseButtonData& Data);
		virtual void OnMouseScroll(EMouseScrollDirection Direction);
		virtual void OnWindowResized(std::uint16_t InWidth, std::uint16_t InHeight);
		virtual void OnActorCreated(LUUID Handle, std::weak_ptr<CActor> ActorRef) {}
		virtual void OnActorDeleted(LUUID Handle) {}
		virtual void OnPauseMenuToggled(bool Opened);

		[[nodiscard]] const FGameSpecification& GetSpecification() const { return Spec; }

		void CreatePlayer();
		void SaveScene();
		void MousePickScene();
		void RaycastSceneAtMouse();

		void InitializeSystems();
		void ShutdownSystems();

		virtual void UpdateViewportBounds();
		[[nodiscard]] virtual std::pair<std::uint16_t, std::uint16_t> GetActiveViewportSize() const;
		[[nodiscard]] const std::array<glm::vec2, 2>& GetViewportBounds() { return ViewportBounds; }

		static bool PreSolve(b2ShapeId ShapeA, b2ShapeId ShapeB, b2Vec2 Point, b2Vec2 Normal, void* Ctx);

	protected:
		float DeltaTime = 0.0f;
		FConfig Config;
		std::uint16_t ViewportWidth = 0;
		std::uint16_t ViewportHeight = 0;
		std::array<glm::vec2, 2> ViewportBounds;
		std::filesystem::path LastSceneFilepath{};

		std::shared_ptr<CPlayer> Player = nullptr;
		std::shared_ptr<CScene> Scene = nullptr;
		std::filesystem::path SceneToOpen{};
		bool bOpenSceneNextTick = false;
		bool bCloseSceneNextTick = false;
		bool bRaycastScene = false;
		FLevelData LevelData;

		struct
		{
			Core::FDelegateHandle OnKey;
			Core::FDelegateHandle OnMouseButton;
			Core::FDelegateHandle OnMouseScroll;
			Core::FDelegateHandle OnWindowResized;
			Core::FDelegateHandle OnActorCreated;
			Core::FDelegateHandle OnActorDeleted;
			Core::FDelegateHandle OnPauseMenuOpened;
		} DelegateHandles;

	private:
		std::vector<std::unique_ptr<IGameSystem>> Systems;
		FGameSpecification Spec{};

		static inline CGameInstance* Instance = nullptr;
	};

}
