#include "actor.h"

#include "core/log.h"

namespace platformer2d {

	glm::vec2 CActor::GetPosition() const
	{
		return glm::vec2(TransformComp.Translation.x, TransformComp.Translation.y);
	}

	void CActor::SetPosition(const float X, const float Y)
	{
		TransformComp.Translation.x = X;
		TransformComp.Translation.y = Y;
		LK_DEBUG("Pos=({}, {})", TransformComp.Translation.x, TransformComp.Translation.y);
	}

	void CActor::SetPosition(const glm::vec2& NewPos)
	{
		TransformComp.Translation.x = NewPos.x;
		TransformComp.Translation.y = NewPos.y;
		LK_DEBUG("Pos=({}, {})", TransformComp.Translation.x, TransformComp.Translation.y);
	}

}