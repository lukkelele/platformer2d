#pragma once

#include "core/layer.h"
#include "game/instance.h"
#include "renderer/editorcamera.h"
#include "renderer/texture.h"
#include "physics/events.h"
#include "scene/scene.h"

namespace platformer2d {

	class CEditor : public CGameInstance
	{
	public:
		CEditor();
		~CEditor();

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
		[[nodiscard]] bool IsGamePaused() override;

		std::uint16_t RaycastScene(std::shared_ptr<CScene> TargetScene, std::vector<FHitResult>& HitResults) override;
		std::uint16_t PickSceneAtMouse(std::shared_ptr<CScene> TargetScene, std::vector<FHitResult>& HitResults) override;

		bool Serialize(const std::filesystem::path& OutFile) const override;
		bool Deserialize(const std::filesystem::path& InFile) override;

	private:
		void UpdateEditorViewportState();
		void UpdateEditorViewportBounds();
		void UpdateViewportBounds() override;
		[[nodiscard]] glm::vec2 GetMouseInViewportSpace() override;
		[[nodiscard]] glm::vec2 GetMouseInWorldSpace(const CCamera& Camera) override;

		void CreatePlayer();

		void UI_Level();
		void UI_Player();
		void UI_ViewportTexture();
		void UI_DrawGizmo();
		void UI_Topbar();
		void UI_MainMenubar();
		void UI_LeftSidebar();
		void UI_LevelLauncher();

		void OnWindowResized(uint16_t InWidth, uint16_t InHeight);
		void OnKey(const FKeyData& Data);
		void OnMouseButton(const FMouseButtonData& Data);
		void OnMouseScroll(EMouseScrollDirection Direction);

		void MousePickScene();
		void RaycastScene();
		void SaveScene();

		void PossessPlayerCamera();
		void PossessEditorCamera();
		[[nodiscard]] CEditorCamera* GetEditorCamera() const;

	private:
		std::shared_ptr<CPlayer> Player = nullptr;
		std::shared_ptr<CScene> Scene = nullptr;
		std::shared_ptr<CActor> EditorCameraActor = nullptr;
		bool bUseEditorCamera = true;
		glm::vec2 EditorCameraSavedPos{0.0f, 0.0f};
		float EditorCameraSavedZoom = 0.30f;
		bool bHasSavedEditorCameraState = false;
		bool PendingEditorCameraLerp = true;

		std::filesystem::path SceneToOpen{};
		bool bOpenSceneNextTick = false;
		bool bCloseSceneNextTick = false;

		std::uint16_t EditorViewportWidth = 0;
		std::uint16_t EditorViewportHeight = 0;
		std::array<glm::vec2, 2> EditorViewportBounds{};
		bool bEditorViewportHovered = false;
		bool bEditorViewportFocused = true;

		bool bPendingViewportResize = false;
	};

}
