#pragma once

#include "core/core.h"
#include "physics/bodytype.h"

namespace platformer2d {

	struct FActorSpecification
	{
		std::string Name = "Unnamed";
		EBodyType BodyType = EBodyType::Static;
		glm::vec2 Position = { 0.0f, 0.0f };
		glm::vec2 Size = { 0.50f, 0.50f };
		float GravityScale = 1.0f;
		float Density = 1.0f;
		float Friction = 0.30f;
		float Restitution = 0.0f;
	};

}