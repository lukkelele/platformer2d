#include "editorcamera.h"

#include "core/log.h"
#include "core/settings.h"
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

	void CEditorCamera::Tick(const float DeltaTime, const bool ViewportHovered)
	{
		if (!bActive) {
			return;
		}

		const auto& Input = FSettings::Get().Input;
		const float Sensitivity = std::clamp(Input.MouseSensitivity, 0.050f, 8.0f);
		const float DragSign = Input.bInvertCameraDrag ? -1.0f : 1.0f;

		const bool MiddleDown = CMouse::IsDown(EMouseButton::Middle);
		const glm::vec2 Mouse = CMouse::GetPos();

		if (MiddleDown && !bWasMiddleDown) {
			if (ViewportHovered) {
				bPanning = true;
				LastMousePos = Mouse;
				CancelSwitchLerp();
			}
		} else if (!MiddleDown && bWasMiddleDown) {
			bPanning = false;
		}

		const glm::vec2 Half = GetHalfSize();
		const bool ViewportValid = (ViewportWidth > 0.0f) && (ViewportHeight > 0.0f);

		if (bPanning && ViewportValid) {
			HandlePan(Mouse, Sensitivity, DragSign);
		} else if (bPanning) {
			LastMousePos = Mouse;
		}

		if (Input.bEdgePan && ViewportHovered && !bPanning && ViewportValid) {
			HandleEdgePan(DeltaTime, Mouse, Input.EdgePanSpeed);
		}

		TickSwitchLerp(DeltaTime);

		bWasMiddleDown = MiddleDown;
		Update();
	}

	void CEditorCamera::HandlePan(const glm::vec2& Mouse, const float Sensitivity, const float DragSign)
	{
		const glm::vec2 Half = GetHalfSize();
		const bool ViewportValid = (ViewportWidth > 0.0f) && (ViewportHeight > 0.0f);

		const glm::vec2 Delta = (Mouse - LastMousePos) * (Sensitivity * DragSign);
		const float WorldPerPixelX = (2.0f * Half.x) / ViewportWidth;
		const float WorldPerPixelY = (2.0f * Half.y) / ViewportHeight;

		glm::vec2 Center = GetPosition();
		Center.x -= Delta.x * WorldPerPixelX;
		Center.y += Delta.y * WorldPerPixelY;
		SetPosition(Center);
		LastMousePos = Mouse;
	}

	void CEditorCamera::HandleEdgePan(const float DeltaTime, const glm::vec2& Mouse, const float EdgePanSpeed)
	{
		const glm::vec2 ViewportSize = ViewportMax - ViewportMin;
		if ((ViewportSize.x <= 0.0f) || (ViewportSize.y <= 0.0f)) {
			return;
		}

		const float DistLeft = Mouse.x - ViewportMin.x;
		const float DistRight = ViewportMax.x - Mouse.x;
		const float DistTop = Mouse.y - ViewportMin.y;
		const float DistBottom = ViewportMax.y - Mouse.y;

		glm::vec2 PanDir(0.0f);
		if (DistLeft < EDGE_PAN_MARGIN_PX) {
			PanDir.x = -1.0f * (1.0f - (DistLeft / EDGE_PAN_MARGIN_PX));
		} else if (DistRight < EDGE_PAN_MARGIN_PX) {
			PanDir.x = 1.0f * (1.0f - (DistRight / EDGE_PAN_MARGIN_PX));
		}
		if (DistTop < EDGE_PAN_MARGIN_PX) {
			PanDir.y = 1.0f * (1.0f - (DistTop / EDGE_PAN_MARGIN_PX));
		} else if (DistBottom < EDGE_PAN_MARGIN_PX) {
			PanDir.y = -1.0f * (1.0f - (DistBottom / EDGE_PAN_MARGIN_PX));
		}

		if ((PanDir.x == 0.0f) && (PanDir.y == 0.0f)) {
			return;
		}

		const glm::vec2 Half = GetHalfSize();
		const float WorldPerPixelX = (2.0f * Half.x) / GetViewportWidth();
		const float WorldPerPixelY = (2.0f * Half.y) / GetViewportHeight();
		const float Speed = std::clamp(EdgePanSpeed, 0.50f, 64.0f);

		glm::vec2 Center = GetPosition();
		Center.x += PanDir.x * Speed * WorldPerPixelX * DeltaTime * 60.0f;
		Center.y += PanDir.y * Speed * WorldPerPixelY * DeltaTime * 60.0f;
		SetPosition(Center);
		CancelSwitchLerp();
	}

	void CEditorCamera::SetViewportBounds(const glm::vec2& InMin, const glm::vec2& InMax)
	{
		ViewportMin = InMin;
		ViewportMax = InMax;
	}

	void CEditorCamera::SetLerpEnabled(const bool Enabled)
	{
		bLerpEnabled = Enabled;
	}

	void CEditorCamera::SetLerpSnapDistance(const float Distance)
	{
		LerpSnapDistance = Distance;
	}

	void CEditorCamera::OnMouseScroll(const EMouseScrollDirection Direction)
	{
		if (!bActive) {
			return;
		}

		CancelSwitchLerp();

		const float ZoomSpeed = std::clamp(FSettings::Get().Input.ZoomSpeed, 0.050f, 8.0f);
		float Step = ZOOM_SCROLL_STEP * ZoomSpeed;
		if (CKeyboard::IsKeyDown(EKey::LeftShift) || CKeyboard::IsKeyDown(EKey::RightShift)) {
			Step *= 3.0f;
		}

		const float ZoomDiff = (Direction == EMouseScrollDirection::Up) ? -Step : Step;
		SetZoom(GetZoom() + ZoomDiff);
	}

}

