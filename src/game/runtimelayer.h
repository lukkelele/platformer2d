#pragma once

#include "core/layer.h"
#include "game/instance.h"
#include "renderer/texture.h"
#include "physics/events.h"
#include "scene/scene.h"

namespace platformer2d {

	class CRuntimeLayer : public CGameInstance
	{
	public:
		CRuntimeLayer();
		~CRuntimeLayer();

		void Initialize() override;
		void Destroy() override;

		void OnAttach() override;
		void OnDetach() override;

		void Tick(float InDeltaTime) override;
		void RenderUI() override;

		[[nodiscard]] CCamera* GetActiveCamera() const override;
		[[nodiscard]] std::shared_ptr<CPlayer> GetPlayer(std::size_t Idx = 0) const override;
		[[nodiscard]] std::shared_ptr<CScene> GetScene() const override { return Scene; }
		void OpenScene(const std::filesystem::path& ScenePath) override;
		void CloseScene() override;

		void PauseGame() override;
		void ResumeGame() override;
		bool IsGamePaused() override;

		std::uint16_t RaycastScene(std::shared_ptr<CScene> TargetScene, std::vector<FHitResult>& HitResults) override;
		std::uint16_t PickSceneAtMouse(std::shared_ptr<CScene> TargetScene, std::vector<FHitResult>& HitResults) override;

		bool Serialize(const std::filesystem::path& OutFile) const override;
		bool Deserialize(const std::filesystem::path& InFile) override;

		void OnWindowResized(std::uint16_t InWidth, std::uint16_t InHeight);
		void OnKey(const FKeyData& Data);
		void OnMouseButton(const FMouseButtonData& Data);

	private:
		void UI_ViewportTexture();

		void MousePickScene();
		void RaycastScene();
		void SaveScene();
		void HandleUpdatedSceneState(ESceneState NewState);

		void CreatePlayer();

	private:
		std::shared_ptr<CPlayer> Player = nullptr;
		std::shared_ptr<CScene> Scene = nullptr;

		std::filesystem::path SceneToOpen{};
		bool bOpenSceneNextTick = false;
		bool bCloseSceneNextTick = false;
		bool bSceneStateChanged = false;
		bool bRaycastScene = false;

		struct
		{
			glm::vec2 Gravity = {0.0f, -5.0f};
			glm::vec2 PlayerSpawn = {0.0f, 0.0f};
			float SceneLoadCameraZoom = 0.30f;
		} LevelData;
	};

}
