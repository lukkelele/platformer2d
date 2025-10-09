#pragma once

#include <string>

#include <glm/glm.hpp>

#include "core/core.h"
#include "core/assert.h"

namespace platformer2d {

	class CActor
	{
	public:
		virtual ~CActor() = default;

		const glm::vec2& GetPosition() const { return Pos; }
		void SetPosition(float X, float Y);
		void SetPosition(const glm::vec2& NewPos);

	protected:
		glm::vec2 Pos{ 0.0f, 0.0f };
	};

}
