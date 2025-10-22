#include "actor.h"

#include "core/log.h"

namespace platformer2d {

	CActor::CActor(const FActorSpecification& Spec)
		: Name(Spec.Name)
	{
		LK_DEBUG_TAG("Actor", "Create: {}", Name);
		TransformComp.Translation.x = Spec.Position.x;
		TransformComp.Translation.y = Spec.Position.y;

		Body = std::make_unique<CBody>(Spec);
	}

	void CActor::Tick(const float DeltaTime)
	{
		Body->Tick(DeltaTime);

		const glm::vec2 BodyPos = Body->GetPosition();
		TransformComp.Translation.x = BodyPos.x;
		TransformComp.Translation.y = BodyPos.y;
	}

	glm::vec2 CActor::GetPosition() const
	{
		return glm::vec2(TransformComp.Translation.x, TransformComp.Translation.y);
	}

	void CActor::SetPosition(const float X, const float Y)
	{
		SetPosition({ X, Y });
	}

	void CActor::SetPosition(const glm::vec2& NewPos)
	{
		TransformComp.Translation.x = NewPos.x;
		TransformComp.Translation.y = NewPos.y;
		Body->SetPosition(NewPos);
	}

}