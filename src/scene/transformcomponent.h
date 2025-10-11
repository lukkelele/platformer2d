#pragma once

#include <cstdio>
#include <format>
#include <utility>
#include <string>

#include <glm/glm.hpp>
#include <glm/ext/matrix_common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/norm.hpp>

#include "core/core.h"
#include "core/assert.h"
#include "core/log_formatters.h"

namespace platformer2d {

	struct FTransformComponent
	{
	public:
		glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };
		float Rotation2D = 0.0f;
		bool bIsStatic = false;

	private:
		glm::vec3 RotationEuler = { 0.0f, 0.0f, 0.0f };
		glm::quat Rotation = { 1.0f, 0.0f, 0.0f, 0.0f };

	public:
		FTransformComponent() = default;
		FTransformComponent(const glm::vec3& Translation) : Translation(Translation) {}
		FTransformComponent(const FTransformComponent& Other) = default;

		glm::vec3 GetTranslation() const { return Translation; }
		glm::vec3 GetScale() const { return Scale; }

		float GetRotation2D() const { return Rotation2D; }

		glm::mat4 GetTransform() const
		{
			return glm::translate(glm::mat4(1.0f), Translation)
				* glm::toMat4(Rotation)
				* glm::scale(glm::mat4(1.0f), Scale);
		}

		glm::mat4 GetInvTransform() const
		{
			const glm::mat4 InvTranslation = glm::translate(glm::mat4(1.0f), -Translation);
			const glm::quat InvRot = glm::conjugate(Rotation);
			const glm::mat4 InvScale = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f / Scale.x, 1.0f / Scale.y, 1.0f / Scale.z));
			return InvScale * glm::toMat4(InvRot) * InvTranslation;
		}

		glm::quat GetRotation() const { return Rotation; }
		glm::vec3 GetRotationEuler() const { return RotationEuler; }

		void SetRotationEuler(const glm::vec3& euler)
		{
			RotationEuler = euler;
			Rotation = glm::quat(RotationEuler);
		}

		void SetRotation(const glm::quat& InQuat)
		{
			Rotation = InQuat;

			const glm::vec3 OriginalEulerer = RotationEuler;
			RotationEuler = glm::eulerAngles(Rotation);

			/* Attempt to avoid 180deg flips in the Euler angles when using SetRotation(quat). */
			if ((std::fabs(RotationEuler.x - OriginalEulerer.x) == glm::pi<float>())
				&& (std::fabs(RotationEuler.z - OriginalEulerer.z) == glm::pi<float>()))
			{
				RotationEuler.x = OriginalEulerer.x;
				RotationEuler.y = glm::pi<float>() - RotationEuler.y;
				RotationEuler.z = OriginalEulerer.z;
			}
		}

		bool IsStatic() const { return bIsStatic; }

		std::string ToString() const
		{
			return LK_FMT("Translation={} Scale={} RotEuler={}", Translation, Scale, RotationEuler);
		}
	};

}