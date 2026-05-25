#include "camera.h"

#include <cmath>

#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/norm.hpp>

#include "renderer/debugrenderer.h"

namespace platformer2d {

	CCamera::CCamera(const float InWidth, const float InHeight, const float InNearP, const float InFarP)
		: ViewportWidth(InWidth)
		, ViewportHeight(InHeight)
		, OrthographicNear(InNearP)
		, OrthographicFar(InFarP)
		, AspectRatio(InWidth / InHeight)
	{
		UpdateView();
		UpdateProjection();
	}

	void CCamera::Update()
	{
		UpdateView();
	}

	void CCamera::SetViewportSize(const uint16_t InWidth, const uint16_t InHeight)
	{
		ViewportWidth = static_cast<float>(InWidth);
		ViewportHeight = static_cast<float>(InHeight);
		AspectRatio = (ViewportWidth / ViewportHeight);

		UpdateProjection();
	}

	void CCamera::SetOrthographic(const float InWidth, const float InHeight, const float InNearClip, const float InFarClip)
	{
		ViewportWidth = InWidth;
		ViewportHeight = InHeight;
		OrthographicNear = InNearClip;
		OrthographicFar = InFarClip;
	}

	void CCamera::SetActive(const bool InActive)
	{
		if (bActive == InActive) {
			return;
		}

		bActive = InActive;
		if (!bActive) {
			CancelSwitchLerp();
		}
	}

	void CCamera::SetZoom(const float InZoom)
	{
		Zoom = std::max(ZOOM_MIN, std::min(ZOOM_MAX, InZoom));
		UpdateProjection();
	}

	void CCamera::BeginSwitchLerp(const glm::vec2& StartPos, const float StartZoom)
	{
		if (!bSwitchLerping) {
			SwitchTargetPos = Center;
			SwitchTargetZoom = Zoom;
			CDebugRenderer::SetDrawBounds(GetPosition(), GetHalfSize());
		}

		Center = StartPos;
		Zoom = std::max(ZOOM_MIN, std::min(ZOOM_MAX, StartZoom));
		UpdateView();
		UpdateProjection();

		bSwitchLerping = true;
	}

	void CCamera::TickSwitchLerp(const float Dt)
	{
		if (!bSwitchLerping || (Dt <= 0.0f)) {
			return;
		}

		const float T = 1.0f - std::exp(-SWITCH_LERP_SPEED * Dt);
		Center += (SwitchTargetPos - Center) * T;
		Zoom += (SwitchTargetZoom - Zoom) * T;

		if ((glm::length(SwitchTargetPos - Center) < SWITCH_LERP_EPSILON_POS)
			&& (std::abs(SwitchTargetZoom - Zoom) < SWITCH_LERP_EPSILON_ZOOM)) {
			Center = SwitchTargetPos;
			Zoom = SwitchTargetZoom;
			bSwitchLerping = false;
		}

		UpdateView();
		UpdateProjection();
		CDebugRenderer::SetDrawBounds(GetPosition(), GetHalfSize());
	}

	void CCamera::Target(const glm::vec2& TargetPos, const float DeltaTime)
	{
		/* Dead-zone logic. */
		if (DeltaTime > 0.0f) {
			glm::vec2 Center2 = glm::vec2(Center.x, Center.y);
			glm::vec2 Offset = TargetPos - Center2;
			glm::vec2 Desired = Center2;

			if (Offset.x > DeadzoneHalf.x) {
				Desired.x = TargetPos.x - DeadzoneHalf.x;
			}
			if (Offset.x < -DeadzoneHalf.x) {
				Desired.x = TargetPos.x + DeadzoneHalf.x;
			}
			if (Offset.y > DeadzoneHalf.y) {
				Desired.y = TargetPos.y - DeadzoneHalf.y;
			}
			if (Offset.y < -DeadzoneHalf.y) {
				Desired.y = TargetPos.y + DeadzoneHalf.y;
			}

			const float T = 1.0f - std::exp(-FollowSpeed * DeltaTime);
			Center2 += (Desired - Center2) * T;

			Center.x = Center2.x;
			Center.y = Center2.y;
		} else {
			/* Instant target lock. */
			Center.x = TargetPos.x;
			Center.y = TargetPos.y;
		}
	}

	void CCamera::SetFollowSpeed(const float InFollowSpeed)
	{
		FollowSpeed = InFollowSpeed;
	}

	void CCamera::SetDeadzone(const glm::vec2& InDeadzone)
	{
		DeadzoneHalf = InDeadzone;
	}

	glm::vec2 CCamera::GetHalfSize() const
	{
		const float HalfHeight = (OrthographicSize * Zoom) * 0.50f;
		const float HalfWidth = HalfHeight * AspectRatio;
		return glm::vec2(HalfWidth, HalfHeight);
	}

	std::pair<float, float> CCamera::GetMinRange() const
	{
		const float HalfHeight = (OrthographicSize * Zoom) * 0.50f;
		const float HalfWidth = HalfHeight * AspectRatio;
		return std::make_pair(-HalfWidth, -HalfHeight);
	}

	std::pair<float, float> CCamera::GetMaxRange() const
	{
		const float HalfHeight = (OrthographicSize * Zoom) * 0.50f;
		const float HalfWidth = HalfHeight * AspectRatio;
		return std::make_pair(HalfWidth, HalfHeight);
	}

	std::pair<glm::vec2, glm::vec2> CCamera::GetMinMaxRange() const
	{
		const float HalfHeight = (OrthographicSize * Zoom) * 0.50f;
		const float HalfWidth = HalfHeight * AspectRatio;
		return std::make_pair(
			glm::vec2(-HalfWidth, -HalfHeight),
			glm::vec2(HalfWidth, HalfHeight));
	}
}

