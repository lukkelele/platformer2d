#include "camera.h"

#include <glm/ext/matrix_common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/norm.hpp>

namespace platformer2d {

	CCamera::CCamera(const float InWidth, const float InHeight, 
					 const float InNearP, const float InFarP)
	{
		ProjectionMatrix = glm::ortho(
			-(InWidth * 0.50f), (InWidth * 0.50f),
			-(InHeight * 0.50f), (InHeight * 0.50f),
			InNearP, 
			InFarP
		);
	}

	glm::quat CCamera::GetOrientation() const
	{
		return glm::quat(glm::vec3(-Pitch - PitchDelta, -Yaw - YawDelta, 0.0f));
	}

	glm::vec3 CCamera::GetUpDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	glm::vec3 CCamera::GetRightDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
	}

	glm::vec3 CCamera::GetForwardDirection() const
	{
		return glm::vec3(glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f)));
	}

	void CCamera::SetOrthoProjection(const float InWidth, const float InHeight, 
									 const float InNearP, const float InFarP)
	{
		ProjectionMatrix = glm::ortho(
			-(InWidth * 0.50f), (InWidth * 0.50f),
			-(InHeight * 0.50f), (InHeight * 0.50f),
			InNearP, 
			InFarP
		);
	}

}
