#pragma once

#include <box2d/box2d.h>
#include <glm/glm.hpp>

#include "core/core.h"

namespace platformer2d {

	enum class EBodyType
	{
		Static,
		Dynamic,
		Kinematic,
	};

	class CBody
	{
	public:
		CBody(EBodyType InType = EBodyType::Static);
		~CBody() = default;

		glm::vec2 GetPosition() const;
		void SetPosition(const glm::vec2& Pos) const;

		float GetRotation() const;
		void SetRotation(float AngleRad) const;

		glm::vec2 GetLinearVelocity() const;
		void SetLinearVelocity(const glm::vec2& Velocity);

	private:
		EBodyType Type;
		b2BodyId ID;
		b2Polygon Box;
		/** @fixme Might be redundant to store these all as members */
		b2ShapeDef ShapeDef;
		b2ShapeId ShapeID;
	};

}
