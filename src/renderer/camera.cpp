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
		//ProjectionMatrix = glm::ortho(
		//	-(InWidth * 0.50f), (InWidth * 0.50f),
		//	-(InHeight * 0.50f), (InHeight * 0.50f),
		//	InNearP, 
		//	InFarP
		//);

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

}
