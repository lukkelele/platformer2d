#pragma once

#include "camera.h"

namespace platformer2d {

	class CEditorCamera : public CCamera
	{
	public:
		CEditorCamera(float InWidth, float InHeight);
		~CEditorCamera() = default;

		void Tick(float DeltaTime, bool ViewportHovered);
		void SetViewportBounds(const glm::vec2& InMin, const glm::vec2& InMax);

		void SetActive(bool InActive);
		[[nodiscard]] bool IsActive() const { return bActive; }

		[[nodiscard]] bool IsLerpEnabled() const { return bLerpEnabled; }
		void SetLerpEnabled(bool Enabled);
		[[nodiscard]] float GetLerpSnapDistance() const { return LerpSnapDistance; }
		void SetLerpSnapDistance(float Distance);

		void OnMouseScroll(EMouseScrollDirection Direction);

	private:
		void HandlePan(const glm::vec2& Mouse, float Sensitivity, float DragSign);
		void HandleEdgePan(float DeltaTime, const glm::vec2& Mouse, float EdgePanSpeed);

	public:
		static constexpr float DEFAULT_LERP_SNAP_DISTANCE = 4.0f;
		static constexpr float ZOOM_SCROLL_STEP = CCamera::ZOOM_DIFF * 5.0f;
		static constexpr float EDGE_PAN_MARGIN_PX = 64.0f;

	private:
		bool bActive = false;
		bool bLerpEnabled = true;
		float LerpSnapDistance = DEFAULT_LERP_SNAP_DISTANCE;

		bool bPanning = false;
		bool bWasMiddleDown = false;
		glm::vec2 LastMousePos{0.0f};

		glm::vec2 ViewportMin{0.0f};
		glm::vec2 ViewportMax{0.0f};
	};

}
