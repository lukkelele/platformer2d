#pragma once

#include "core/layer.h"
#include "game/instance.h"
#include "renderer/texture.h"
#include "physics/events.h"
#include "scene/scene.h"

namespace platformer2d {

	class CEditor : public CGameInstance
	{
	public:
		CEditor();
		virtual ~CEditor() = default;

		virtual void Initialize() override;
		virtual void Destroy() override;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void Tick(float InDeltaTime) override;
		virtual CCamera* GetActiveCamera() const override;
		virtual CPlayer* GetPlayer(std::size_t Idx = 0) const override;
		virtual std::shared_ptr<CScene> GetScene() const override { return Scene; }

		virtual uint16_t RaycastScene(std::shared_ptr<CScene> TargetScene, std::vector<FHitResult>& HitResults) override;
		virtual uint16_t PickSceneAtMouse(std::shared_ptr<CScene> TargetScene, std::vector<FHitResult>& HitResults) override;

		virtual void RenderUI() override;

		virtual bool Serialize(const std::filesystem::path& OutFile) const override;
		virtual bool Deserialize(const std::filesystem::path& InFile) override;

	private:
		void UpdateEditorViewportState();
		void UpdateEditorViewportBounds();
		virtual void UpdateViewportBounds() override;
		virtual glm::vec2 GetMouseInViewportSpace() override;
		virtual glm::vec2 GetMouseInWorldSpace(const CCamera& Camera) override;

		void CreatePlayer();

		void UI_Level();
		void UI_Player();
		void UI_ViewportTexture();
		void UI_DrawGizmo();

		void OnWindowResized(uint16_t InWidth, uint16_t InHeight);
		void OnKeyPressed(const FKeyData& Data);
		void OnMouseButtonPressed(const FMouseButtonData& Data);
		void OnSensorBeginEvent(const CSensorBeginEvent& Event);
		void OnSensorEndEvent(const CSensorEndEvent& Event);

		void MousePickScene();
		void RaycastScene();

		void UI_PrepareEditorViewport();

	private:
		std::shared_ptr<CPlayer> Player = nullptr;
		std::shared_ptr<CScene> Scene = nullptr;

		uint16_t EditorViewportWidth = 0;
		uint16_t EditorViewportHeight = 0;
		std::array<glm::vec2, 2> EditorViewportBounds{};
		bool bEditorViewportHovered = false;
		bool bEditorViewportFocused = true;
	};

}
