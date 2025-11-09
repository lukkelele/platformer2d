#include "camera.h"

#include <cmath>

#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/norm.hpp>

namespace platformer2d {

	CCamera::CCamera(const float InWidth, const float InHeight, 
					 const float InNearP, const float InFarP)
		: OrthographicNear(InNearP)
		, OrthographicFar(InFarP)
		, AspectRatio(InWidth / InHeight)
	{
		UpdateView();
		UpdateProjection();

		CKeyboard::OnKeyPressed.Add(this, &CCamera::OnKeyPressed);
		CMouse::OnButtonPressed.Add(this, &CCamera::OnMouseButtonPressed);
		CMouse::OnScrolled.Add(this, &CCamera::OnMouseScrolled);
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

	void CCamera::SetZoom(const float InZoom)
	{
		Zoom = std::max(ZOOM_MIN, std::min(ZOOM_MAX, InZoom));
		UpdateProjection();
	}

	void CCamera::OnKeyPressed(const FKeyData& KeyData)
	{
		LK_TRACE_TAG("Camera", "OnKeyPressed: {} (Count: {})", Enum::ToString(KeyData.Key), KeyData.RepeatCount);
		if (KeyData.Key == EKey::Minus)
		{
			SetZoom(GetZoom() - ZOOM_DIFF);
		}

		/* Modifiers */
		/* @fixme: This should take the user keyboard layout into account... */
		if (CKeyboard::IsKeyDown(EKey::LeftShift) || CKeyboard::IsKeyDown(EKey::RightShift))
		{
			if (KeyData.Key == EKey::Equal)
			{
				SetZoom(GetZoom() + ZOOM_DIFF);
			}
		}
	}

	void CCamera::Target(const glm::vec2& TargetPos, const float DeltaTime)
	{
		/* Dead-zone logic. */
		if (DeltaTime > 0.0f)
		{
			glm::vec2 Center2 = glm::vec2(Center.x, Center.y);
			glm::vec2 Offset = TargetPos - Center2;
			glm::vec2 Desired = Center2;

			if (Offset.x > DeadzoneHalf.x)
			{
				Desired.x = TargetPos.x - DeadzoneHalf.x;
			}
			if (Offset.x < -DeadzoneHalf.x)
			{
				Desired.x = TargetPos.x + DeadzoneHalf.x;
			}
			if (Offset.y > DeadzoneHalf.y)
			{
				Desired.y = TargetPos.y - DeadzoneHalf.y;
			}
			if (Offset.y < -DeadzoneHalf.y)
			{
				Desired.y = TargetPos.y + DeadzoneHalf.y;
			}

			const float T = 1.0f - std::exp(-FollowSpeed * DeltaTime);
			Center2 += (Desired - Center2) * T;

			Center.x = Center2.x;
			Center.y = Center2.y;
		}
		else
		{
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
			glm::vec2(HalfWidth, HalfHeight)
		);
	}

	void CCamera::OnMouseButtonPressed(const FMouseButtonData& ButtonData)
	{
		LK_TRACE_TAG("Camera", "Mouse button: {}", Enum::ToString(ButtonData.Button));
	}

	void CCamera::OnMouseScrolled(const EMouseScrollDirection Direction)
	{
		LK_TRACE_TAG("Camera", "Mouse scroll direction: {}", Enum::ToString(Direction));
	}

}
