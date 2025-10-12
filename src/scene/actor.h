#pragma once

#include <string>

#include "core/core.h"
#include "core/assert.h"

#include "transformcomponent.h"

namespace platformer2d {

	class CActor
	{
	public:
		virtual ~CActor() = default;

		glm::vec2 GetPosition() const;
		void SetPosition(float X, float Y);
		void SetPosition(const glm::vec2& NewPos);

		inline FTransformComponent& GetTransformComponent() { return TransformComp; }
		inline const FTransformComponent& GetTransformComponent() const { return TransformComp; }
		operator FTransformComponent&() { return TransformComp; }
		operator const FTransformComponent&() const { return TransformComp; }

	protected:
		FTransformComponent TransformComp{};
	};

}
