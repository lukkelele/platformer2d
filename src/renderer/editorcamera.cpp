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

	CEditorCamera::~CEditorCamera()
	{
		if (bActive) {
			CMouse::OnScrolled.Remove(OnMouseScrolledHandle);
		}
	}

	void CEditorCamera::SetActive(const bool InActive)
	{
		if (bActive == InActive) {
			return;
		}

		bActive = InActive;
		if (bActive) {
			OnMouseScrolledHandle = CMouse::OnScrolled.Add(this, &CEditorCamera::OnMouseScrolled);
		} else {
			CMouse::OnScrolled.Remove(OnMouseScrolledHandle);
			bPanning = false;
			bWasMiddleDown = false;
			bSwitchLerping = false;
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

		if (bSwitchLerping && (RealDeltaTime > 0.0f)) {
			const float T = 1.0f - std::exp(-SwitchLerpSpeed * RealDeltaTime);

			const glm::vec2 Pos = GetPosition();
			const glm::vec2 NewPos = Pos + (SwitchTargetPos - Pos) * T;
			SetPosition(NewPos);

			const float Zoom = GetZoom();
			const float NewZoom = Zoom + (SwitchTargetZoom - Zoom) * T;
			SetZoom(NewZoom);

			if ((glm::length(SwitchTargetPos - NewPos) < SwitchLerpEpsilonPos)
				&& (std::abs(SwitchTargetZoom - NewZoom) < SwitchLerpEpsilonZoom)) {
				SetPosition(SwitchTargetPos);
				SetZoom(SwitchTargetZoom);
				bSwitchLerping = false;
			}
		}

		bWasMiddleDown = MiddleDown;
		Update();
	}

	void CEditorCamera::BeginSwitchLerp(const glm::vec2& StartPos, const float StartZoom)
	{
		SwitchTargetPos = GetPosition();
		SwitchTargetZoom = GetZoom();

		if (!bLerpEnabled) {
			bSwitchLerping = false;
			return;
		}

		const float Distance = glm::length(SwitchTargetPos - StartPos);
		if (Distance > LerpSnapDistance) {
			bSwitchLerping = false;
			return;
		}

		SetPosition(StartPos);
		SetZoom(StartZoom);
		bSwitchLerping = true;
	}

	void CEditorCamera::SetLerpEnabled(const bool Enabled)
	{
		bLerpEnabled = Enabled;
	}

	void CEditorCamera::SetLerpSnapDistance(const float Distance)
	{
		LerpSnapDistance = Distance;
	}

	void CEditorCamera::CancelSwitchLerp()
	{
		if (bSwitchLerping) {
			bSwitchLerping = false;
		}
	}

	void CEditorCamera::OnMouseScrolled(const EMouseScrollDirection Direction)
	{
		if (!bActive) {
			return;
		}

		CancelSwitchLerp();

		float Step = ZoomScrollStep;
		if (CKeyboard::IsKeyDown(EKey::LeftShift) || CKeyboard::IsKeyDown(EKey::RightShift)) {
			Step *= 3.0f;
		}

		const float ZoomDiff = (Direction == EMouseScrollDirection::Up) ? -Step : Step;
		SetZoom(GetZoom() + ZoomDiff);
	}

}
