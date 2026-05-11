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

		void OnSensorBeginEvent(const CSensorBeginEvent& Event) override;
		void OnSensorEndEvent(const CSensorEndEvent& Event) override;
		void OnContactBeginEvent(const CContactBeginEvent& Event) override;
		void OnContactEndEvent(const CContactEndEvent& Event) override;

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
		void OnKeyPressed(const FKeyData& Data);
		void OnMouseButtonPressed(const FMouseButtonData& Data);
		void OnMouseScrolled(EMouseScrollDirection Direction);

		void MousePickScene();
		void RaycastScene();
		void SaveScene();

		void PossessEditorCamera();
		void PossessPlayerCamera();

		void OnPickupEvent(CPlayer& InPlayer, const FInteractionComponent& IC);
		void OnPickupEvent_Item(const FPickupInteraction& Interaction, CPlayer& InPlayer);
		void OnPickupEvent_Rifle(const FPickupInteraction& Interaction, CPlayer& InPlayer);

	private:
		std::shared_ptr<CPlayer> Player = nullptr;
		std::shared_ptr<CScene> Scene = nullptr;
		std::shared_ptr<CActor> EditorCameraActor = nullptr;
		bool bUseEditorCamera = true;
		glm::vec2 EditorCameraSavedPos{0.0f, 0.0f};
		float EditorCameraSavedZoom = 0.30f;
		bool bHasSavedEditorCameraState = false;
		bool PendingEditorCameraLerp = true;

		CEditorCamera* GetEditorCamera() const;

		std::filesystem::path LastSceneFilepath{};
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
