#include "editorcamera.h"

#include "core/log.h"
#include "core/input/keyboard.h"
#include "core/input/mouse.h"

namespace platformer2d {

	CEditorCamera::CEditorCamera(const float InWidth, const float InHeight)
		: CCamera(InWidth, InHeight)
	{
		LK_TRACE_TAG("EditorCamera", "Created ({}x{})", InWidth, InHeight);
	}

	void CEditorCamera::SetActive(const bool InActive)
	{
		if (bActive == InActive) {
			return;
		}

		bActive = InActive;
		if (!bActive) {
			bPanning = false;
			bWasMiddleDown = false;
			CancelSwitchLerp();
		}
	}

	void CEditorCamera::Tick(const float RealDeltaTime, const bool ViewportHovered)
	{
		if (!bActive) {
			return;
		}

		const bool MiddleDown = CMouse::IsDown(EMouseButton::Middle);
		const auto [MX, MY] = CMouse::GetPos();
		const glm::vec2 Mouse{MX, MY};

		if (MiddleDown && !bWasMiddleDown) {
			if (ViewportHovered) {
				bPanning = true;
				LastMousePos = Mouse;
				CancelSwitchLerp();
			}
		} else if (!MiddleDown && bWasMiddleDown) {
			bPanning = false;
		}

		if (bPanning) {
			const glm::vec2 Delta = Mouse - LastMousePos;
			const glm::vec2 Half = GetHalfSize();
			const float ViewW = GetViewportWidth();
			const float ViewH = GetViewportHeight();
			if ((ViewW > 0.0f) && (ViewH > 0.0f)) {
				const float WorldPerPixelX = (2.0f * Half.x) / ViewW;
				const float WorldPerPixelY = (2.0f * Half.y) / ViewH;

				glm::vec2 Center = GetPosition();
				Center.x -= Delta.x * WorldPerPixelX;
				Center.y += Delta.y * WorldPerPixelY;
				SetPosition(Center);
			}
			LastMousePos = Mouse;
		}

		TickSwitchLerp(RealDeltaTime);

		bWasMiddleDown = MiddleDown;
		Update();
	}

	void CEditorCamera::SetLerpEnabled(const bool Enabled)
	{
		bLerpEnabled = Enabled;
	}

	void CEditorCamera::SetLerpSnapDistance(const float Distance)
	{
		LerpSnapDistance = Distance;
	}

	void CEditorCamera::OnMouseScrolled(const EMouseScrollDirection Direction)
	{
		if (!bActive) {
			return;
		}

		CancelSwitchLerp();

		float Step = ZOOM_SCROLL_STEP;
		if (CKeyboard::IsKeyDown(EKey::LeftShift) || CKeyboard::IsKeyDown(EKey::RightShift)) {
			Step *= 3.0f;
		}

		const float ZoomDiff = (Direction == EMouseScrollDirection::Up) ? -Step : Step;
		SetZoom(GetZoom() + ZoomDiff);
	}

}

