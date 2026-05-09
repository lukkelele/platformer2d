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

		void BeginSwitchLerp(const glm::vec2& StartPos, float StartZoom);
		[[nodiscard]] bool IsLerpEnabled() const { return bLerpEnabled; }
		void SetLerpEnabled(bool Enabled);
		[[nodiscard]] float GetLerpSnapDistance() const { return LerpSnapDistance; }
		void SetLerpSnapDistance(float Distance);

	public:
		static constexpr float DefaultLerpSnapDistance = 4.0f;
		static constexpr float ZoomScrollStep = CCamera::ZOOM_DIFF * 5.0f;
		static constexpr float SwitchLerpSpeed = 9.0f;
		static constexpr float SwitchLerpEpsilonPos = 0.005f;
		static constexpr float SwitchLerpEpsilonZoom = 0.0005f;

	private:
		void OnMouseScrolled(EMouseScrollDirection Direction);
		void CancelSwitchLerp();

	private:
		bool bActive = false;
		bool bLerpEnabled = true;
		float LerpSnapDistance = DefaultLerpSnapDistance;

		bool bPanning = false;
		bool bWasMiddleDown = false;
		glm::vec2 LastMousePos{0.0f};

		bool bSwitchLerping = false;
		glm::vec2 SwitchTargetPos{0.0f};
		float SwitchTargetZoom = 0.30f;

		Core::FDelegateHandle OnMouseScrolledHandle;
	};

}
