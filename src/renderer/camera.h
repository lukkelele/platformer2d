#pragma once

#include <glm/glm.hpp>

#include "core/assert.h"
#include "core/core.h"

namespace platformer2d {

	class CCamera
	{
	public:
		CCamera(float InWidth, float InHeight, float InNearP, float InFarP);
		CCamera() = delete;
		virtual ~CCamera() = default;

		inline const glm::mat4& GetViewMatrix() const { return ViewMatrix; }
		inline const glm::mat4& GetProjectionMatrix() const { return ProjectionMatrix; }
		inline glm::mat4 GetViewProjection() const { return GetProjectionMatrix() * ViewMatrix; }
		inline float GetRotation() const { return glm::radians(Rotation); }
		inline float GetRotationSpeed() const { return RotationSpeed; }
		const glm::vec3& GetOrigin() const { return Origin; }
		const glm::vec3& GetFocalPoint() const { return FocalPoint; }
		const glm::vec3& GetDirection() const { return Direction; }

		glm::quat GetOrientation() const;
		glm::vec3 GetUpDirection() const;
		glm::vec3 GetRightDirection() const;
		glm::vec3 GetForwardDirection() const;

		void SetOrthoProjection(float InWidth, float InHeight, float InNearP, float InFarP);

	private:
		glm::vec3 Origin = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Direction{};
		glm::vec3 FocalPoint{};
		glm::vec3 PositionDelta = { 0.0f, 0.0f, 0.0f };

		float Pitch = 0.0f;
		float PitchDelta = 0.0f;
		float Yaw = 0.0f;
		float YawDelta = 0.0f;

		float OrthographicSize = 10.0f;
		float OrthographicNear = -1.0f;
		float OrthographicFar = 1.0f;

		float Rotation = 0.0f;
		float RotationSpeed = 0.280f;

		glm::mat4 ViewMatrix = glm::mat4(1.0f);
		glm::mat4 ProjectionMatrix = glm::mat4(1.0f);
	};

}