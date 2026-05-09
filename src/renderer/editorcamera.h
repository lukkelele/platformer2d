#pragma once

#include "camera.h"

namespace platformer2d {

	class CEditorCamera : public CCamera
	{
	public:
		CEditorCamera(float InWidth, float InHeight);
		~CEditorCamera();

		void Tick(float RealDeltaTime, bool ViewportHovered);

		void SetActive(bool InActive);
		[[nodiscard]] bool IsActive() const { return bActive; }

		[[nodiscard]] bool IsLerpEnabled() const { return bLerpEnabled; }
		void SetLerpEnabled(bool Enabled);
		[[nodiscard]] float GetLerpSnapDistance() const { return LerpSnapDistance; }
		void SetLerpSnapDistance(float Distance);

	public:
		static constexpr float DEFAULT_LERP_SNAP_DISTANCE = 4.0f;
		static constexpr float ZOOM_SCROLL_STEP = CCamera::ZOOM_DIFF * 5.0f;

	private:
		void OnMouseScrolled(EMouseScrollDirection Direction);

	private:
		bool bActive = false;
		bool bLerpEnabled = true;
		float LerpSnapDistance = DEFAULT_LERP_SNAP_DISTANCE;

		bool bPanning = false;
		bool bWasMiddleDown = false;
		glm::vec2 LastMousePos{0.0f};

		Core::FDelegateHandle OnMouseScrolledHandle;
	};

}
