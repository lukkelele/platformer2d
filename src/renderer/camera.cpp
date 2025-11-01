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
		, AspectRatio(InWidth/InHeight)
	{
		UpdateView();
		UpdateProjection();

		CKeyboard::OnKeyPressed.Add(this, &CCamera::OnKeyPressed);
		CMouse::OnMouseButtonPressed.Add(this, &CCamera::OnMouseButtonPressed);
		CMouse::OnMouseScrolled.Add(this, &CCamera::OnMouseScrolled);
	}

	void CCamera::Update()
	{
		UpdateView();
	}

	void CCamera::SetViewportSize(const uint16_t InWidth, const uint16_t InHeight)
	{
		ViewportWidth = static_cast<float>(InWidth);
		ViewportHeight = static_cast<float>(InHeight);

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
		static constexpr float ZoomDiff = 0.010f;
		if (KeyData.Key == EKey::Minus)
		{
			SetZoom(GetZoom() - ZoomDiff);
		}

		/* Modifiers */
		/* @fixme: This should take the user keyboard layout into account... */
		if (CKeyboard::IsKeyDown(EKey::LeftShift) || CKeyboard::IsKeyDown(EKey::RightShift))
		{
			if (KeyData.Key == EKey::Equal)
			{
				SetZoom(GetZoom() + ZoomDiff);
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

	void CCamera::OnMouseButtonPressed(const FMouseButtonData& ButtonData)
	{
		LK_TRACE_TAG("Camera", "Mouse button: {}", Enum::ToString(ButtonData.Button));
	}

	void CCamera::OnMouseScrolled(const EMouseScrollDirection Direction)
	{
		LK_TRACE_TAG("Camera", "Mouse scroll direction: {}", Enum::ToString(Direction));
	}

}
