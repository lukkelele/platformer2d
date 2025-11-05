#pragma once

#include <glm/glm.hpp>
#include <glm/ext/matrix_common.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "core/assert.h"
#include "core/core.h"
#include "core/input/keyboard.h"
#include "core/input/mouse.h"

namespace platformer2d {

	class CCamera
	{
	public:
		CCamera(float InWidth, float InHeight, float InNearP = -1.0f, float InFarP = 1.0f);
		CCamera() = delete;
		virtual ~CCamera() = default;

		void Update();
		void SetViewportSize(uint16_t InWidth, uint16_t InHeight);

		inline const glm::mat4& GetViewMatrix() const { return ViewMatrix; }
		inline const glm::mat4& GetProjectionMatrix() const { return ProjectionMatrix; }
		inline glm::mat4 GetViewProjection() const { return GetProjectionMatrix() * ViewMatrix; }
		inline float GetRotation() const { return glm::radians(Rotation); }
		inline float GetRotationSpeed() const { return RotationSpeed; }

		void SetOrthographic(float InWidth, float InHeight, float InNearClip = -1.0f, float InFarClip = 1.0f);

		void SetZoom(float InZoom);
		float GetZoom() const { return Zoom; }	

		void Target(const glm::vec2& TargetPos, float DeltaTime = 0.0f);
		void SetFollowSpeed(float InFollowSpeed);
		void SetDeadzone(const glm::vec2& InDeadzone);
		glm::vec2 GetHalfSize() const;

		std::pair<float, float> GetMinRange() const;
		std::pair<float, float> GetMaxRange() const;
		std::pair<glm::vec2, glm::vec2> GetMinMaxRange() const;

	private:
		FORCEINLINE void UpdateProjection()
		{
			const float HalfHeight = (OrthographicSize * Zoom) * 0.50f;
			const float HalfWidth = HalfHeight * AspectRatio;

			ProjectionMatrix = glm::ortho(
				-HalfWidth,  HalfWidth,
				-HalfHeight, HalfHeight,
				OrthographicNear, OrthographicFar
			);
		}

		FORCEINLINE void UpdateView()
		{
			ViewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-Center.x, -Center.y, 0.0f));
		}

		void OnKeyPressed(const FKeyData& KeyData);
		void OnMouseButtonPressed(const FMouseButtonData& ButtonData);
		void OnMouseScrolled(const EMouseScrollDirection Direction);

	public:
		static inline constexpr float ZOOM_MIN = 0.010f;
		static inline constexpr float ZOOM_MAX = 1.0f;
		static inline constexpr float ZOOM_DIFF = 0.010f;
	private:
		glm::vec2 Center = { 0.0f, 0.0f };

		float ViewportWidth;
		float ViewportHeight;

		float OrthographicSize = 10.0f;
		float OrthographicNear = -1.0f;
		float OrthographicFar = 1.0f;
		float AspectRatio;

		float Zoom = 0.25f;

		float Rotation = 0.0f;
		float RotationSpeed = 0.280f;

		glm::mat4 ViewMatrix = glm::mat4(1.0f);
		glm::mat4 ProjectionMatrix = glm::mat4(1.0f);

		glm::vec2 DeadzoneHalf = { 0.10f, 0.0f }; /* World units. */
		float FollowSpeed = 10.0f;
	};

}