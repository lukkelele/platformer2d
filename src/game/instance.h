#pragma once

#include "core/core.h"
#include "core/layer.h"
#include "player.h"
#include "physics/events.h"
#include "physics/physicsworld.h"
#include "physics/ray.h"

namespace platformer2d {

	class CScene;

	struct FConfig
	{
		struct
		{
			bool bDrawRayHits = false;
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

	class CGameInstance : public CLayer, public ISerializable<ESerializable::File>
	{
	public:
		CGameInstance(CGameInstance* InstanceRef, const FGameSpecification& InSpec);
		CGameInstance() = delete;
		virtual ~CGameInstance();

		virtual void Initialize() = 0;
		virtual void Destroy() = 0;

		virtual void Tick(float InDeltaTime) override = 0;
		virtual void RenderUI() override = 0;

		virtual CCamera* GetActiveCamera() const = 0;
		virtual std::shared_ptr<CPlayer> GetPlayer(std::size_t Idx = 0) const = 0;
		virtual std::shared_ptr<CScene> GetScene() const = 0;
		virtual void OpenScene(const std::filesystem::path& ScenePath) = 0;
		virtual void CloseScene() = 0;
		bool HasScene() const { return GetScene() != nullptr; }

		virtual void PauseGame() = 0;
		virtual void ResumeGame() = 0;
		virtual bool IsGamePaused() = 0;

		virtual uint16_t RaycastScene(std::shared_ptr<CScene> TargetScene, std::vector<FHitResult>& HitResults) = 0;
		virtual uint16_t PickSceneAtMouse(std::shared_ptr<CScene> TargetScene, std::vector<FHitResult>& HitResults) = 0;

		/**
		 * @brief Get mouse position in viewport space.
		 * Range: (-1, 1)
		 */
		virtual glm::vec2 GetMouseInViewportSpace();

		/**
		 * @brief Get mouse position in world space.
		 */
		virtual glm::vec2 GetMouseInWorldSpace(const CCamera& Camera);

		float GetDeltaTime() const { return DeltaTime; }

		virtual void OnSensorBeginEvent(const CSensorBeginEvent& Event);
		virtual void OnSensorEndEvent(const CSensorEndEvent& Event);
		virtual void OnContactBeginEvent(const CContactBeginEvent& Event);
		virtual void OnContactEndEvent(const CContactEndEvent& Event);

		virtual bool Serialize(const std::filesystem::path& OutFile) const override = 0;
		virtual bool Deserialize(const std::filesystem::path& InFile) override = 0;

		static bool PreSolve(b2ShapeId ShapeA, b2ShapeId ShapeB, b2Vec2 Point, b2Vec2 Normal, void* Ctx);

		virtual void OnPickupEvent(CPlayer& InPlayer, const FInteractionComponent& IC);
		virtual void OnPickupEvent_Item(const FPickupInteraction& Interaction, CPlayer& InPlayer);
		virtual void OnPickupEvent_Rifle(const FPickupInteraction& Interaction, CPlayer& InPlayer);

		static CGameInstance* Get() { return Instance; }

	protected:
		const FGameSpecification& GetSpecification() const { return Spec; }

		void BindPhysicsEvents();
		void UnbindPhysicsEvents();

		virtual void UpdateViewportBounds();
		const std::array<glm::vec2, 2>& GetViewportBounds() { return ViewportBounds; }

	protected:
		float DeltaTime = 0.0f;
		FConfig Config;
		uint16_t ViewportWidth = 0;
		uint16_t ViewportHeight = 0;
		std::array<glm::vec2, 2> ViewportBounds;
		std::filesystem::path LastSceneFilepath{};

		struct
		{
			Core::FDelegateHandle OnWindowResized;
			Core::FDelegateHandle OnKey;
			Core::FDelegateHandle OnMouseButton;
			Core::FDelegateHandle OnMouseScroll;
			Core::FDelegateHandle OnSensorBeginEvent;
			Core::FDelegateHandle OnSensorEndEvent;
			Core::FDelegateHandle OnContactBeginEvent;
			Core::FDelegateHandle OnContactEndEvent;
			Core::FDelegateHandle OnActorCreated;
			Core::FDelegateHandle OnActorDeleted;
			Core::FDelegateHandle OnPauseMenuOpened;
		} DelegateHandles;

	private:
		FGameSpecification Spec{};

		static inline CGameInstance* Instance = nullptr;
	};

}
