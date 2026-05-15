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

		void RenderUI() override;

		[[nodiscard]] CCamera* GetActiveCamera() const override;
		std::uint16_t PickSceneAtMouse(std::shared_ptr<CScene> TargetScene, std::vector<FHitResult>& HitResults) override;

		bool Serialize(const std::filesystem::path& OutFile) const override;
		bool Deserialize(const std::filesystem::path& InFile) override;

	private:
		void OnInitialize() override;
		void OnShutdown() override;
		void OnSceneOpened() override;
		void OnSceneClosing() override;
		void OnPreTick(float InDeltaTime) override;
		void OnPostTick(float InDeltaTime) override;
		void OnKey(const FKeyData& Data) override;
		void OnMouseButton(const FMouseButtonData& Data) override;
		void OnMouseScroll(EMouseScrollDirection Direction) override;
		void OnActorCreated(LUUID Handle, std::weak_ptr<CActor> ActorRef) override;
		void OnActorDeleted(LUUID Handle) override;

		[[nodiscard]] std::pair<std::uint16_t, std::uint16_t> GetActiveViewportSize() const override { return {EditorViewportWidth, EditorViewportHeight}; }
		void UpdateViewportBounds() override;
		[[nodiscard]] glm::vec2 GetMouseInViewportSpace() override;

		void UpdateEditorViewportState();
		void UpdateEditorViewportBounds();

		void UI_Level();
		void UI_Player();
		void UI_ViewportTexture();
		void UI_DrawGizmo();
		void UI_Topbar();
		void UI_MainMenubar();
		void UI_LeftSidebar();
		void UI_LevelLauncher();
		void UI_SceneBrowser();
		void UI_BottomBar();

		void SwitchToScene(const std::filesystem::path& NewPath);

		void HandleViewportLeftClick();

		void PossessPlayerCamera();
		void PossessEditorCamera();
		[[nodiscard]] CEditorCamera* GetEditorCamera() const;

	private:
		std::shared_ptr<CActor> EditorCamera = nullptr;
		bool bUseEditorCamera = true;
		glm::vec2 EditorCameraSavedPos{0.0f, 0.0f};
		float EditorCameraSavedZoom = 0.30f;
		bool bHasSavedEditorCameraState = false;
		bool PendingEditorCameraLerp = true;

		std::uint16_t EditorViewportWidth = 0;
		std::uint16_t EditorViewportHeight = 0;
		std::array<glm::vec2, 2> EditorViewportBounds{};
		bool bEditorViewportHovered = false;
		bool bEditorViewportFocused = true;

		bool bPendingViewportResize = false;
		bool bSerializeOnNextSceneOpened = false;
	};

}
