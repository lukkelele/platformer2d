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
		CCamera(CCamera&&) = delete;
		CCamera(const CCamera&) = delete;
		virtual ~CCamera() = default;

		CCamera& operator=(const CCamera&) = delete;
		CCamera& operator=(CCamera&&) = delete;

		void Update();
		void SetViewportSize(uint16_t InWidth, uint16_t InHeight);

		[[nodiscard]] const glm::mat4& GetViewMatrix() const { return ViewMatrix; }
		[[nodiscard]] const glm::mat4& GetProjectionMatrix() const { return ProjectionMatrix; }
		[[nodiscard]] glm::mat4 GetViewProjection() const { return GetProjectionMatrix() * ViewMatrix; }
		[[nodiscard]] glm::vec2 GetPosition() const { return Center; }
		void SetPosition(const glm::vec2& Pos)
		{
			Center = Pos;
			UpdateView();
		}
		[[nodiscard]] float GetRotation() const { return glm::radians(Rotation); }
		[[nodiscard]] float GetRotationSpeed() const { return RotationSpeed; }
		[[nodiscard]] float GetViewportWidth() const { return ViewportWidth; }
		[[nodiscard]] float GetViewportHeight() const { return ViewportHeight; }

		void SetOrthographic(float InWidth, float InHeight, float InNearClip = -1.0f, float InFarClip = 1.0f);

		virtual void SetActive(bool InActive);
		[[nodiscard]] bool IsActive() const { return bActive; }

		void SetZoom(float InZoom);
		[[nodiscard]] float GetZoom() const { return Zoom; }

		void Target(const glm::vec2& TargetPos, float DeltaTime = 0.0f);
		void SetFollowSpeed(float InFollowSpeed);
		void SetDeadzone(const glm::vec2& InDeadzone);
		[[nodiscard]] glm::vec2 GetHalfSize() const;

		void BeginSwitchLerp(const glm::vec2& StartPos, float StartZoom);
		void TickSwitchLerp(float Dt);
		void CancelSwitchLerp() { bSwitchLerping = false; }
		void SetSwitchTargetPos(const glm::vec2& Pos) { SwitchTargetPos = Pos; }
		[[nodiscard]] bool IsSwitchLerping() const { return bSwitchLerping; }

		[[nodiscard]] std::pair<float, float> GetMinRange() const;
		[[nodiscard]] std::pair<float, float> GetMaxRange() const;
		std::pair<glm::vec2, glm::vec2> GetMinMaxRange() const;

		FORCEINLINE void UpdateProjection()
		{
			const float HalfHeight = (OrthographicSize * Zoom) * 0.50f;
			const float HalfWidth = HalfHeight * AspectRatio;

			ProjectionMatrix = glm::ortho(
				-HalfWidth, HalfWidth,
				-HalfHeight, HalfHeight,
				OrthographicNear, OrthographicFar);
		}

		FORCEINLINE void UpdateView()
		{
			ViewMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-Center.x, -Center.y, 0.0f));
		}

	public:
		static constexpr float ZOOM_MIN = 0.010f;
		static constexpr float ZOOM_MAX = 1.0f;
		static constexpr float ZOOM_DIFF = 0.010f;
		static constexpr float SWITCH_LERP_SPEED = 9.0f;
		static constexpr float SWITCH_LERP_EPSILON_POS = 0.005f;
		static constexpr float SWITCH_LERP_EPSILON_ZOOM = 0.0005f;

	protected:
		bool bActive = false;
		float ViewportWidth;
		float ViewportHeight;
		float AspectRatio;

		float OrthographicSize = 10.0f;
		float OrthographicNear = -1.0f;
		float OrthographicFar = 1.0f;

		bool bSwitchLerping = false;
		glm::vec2 SwitchTargetPos{0.0f};
		float SwitchTargetZoom = 0.30f;

	private:
		glm::vec2 Center = {0.0f, 0.0f};

		float Zoom = 0.25f;

		float Rotation = 0.0f;
		float RotationSpeed = 0.280f;

		glm::mat4 ViewMatrix = glm::mat4(1.0f);
		glm::mat4 ProjectionMatrix = glm::mat4(1.0f);

		glm::vec2 DeadzoneHalf = {0.10f, 0.0f}; /* World units. */
		float FollowSpeed = 10.0f;
	};

}
