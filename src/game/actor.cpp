#include "actor.h"

#include "core/log.h"

namespace platformer2d {

	void CActor::SetPosition(const float X, const float Y)
	{
		Pos.x = X;
		Pos.y = Y;
		LK_DEBUG("Pos=({}, {})", Pos.x, Pos.y);
	}

	void CActor::SetPosition(const glm::vec2& NewPos)
	{
		Pos = NewPos;
		LK_DEBUG("Pos=({}, {})", Pos.x, Pos.y);
	}

}