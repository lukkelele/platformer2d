#include "actor.h"

#include "core/log.h"

namespace platformer2d {

	CActor::CActor(const FActorSpecification& Spec)
	{
		Handle = GenerateHandle();
	}

	CActor::CActor(const FBodySpecification& BodySpec)
	{
		Handle = GenerateHandle();

		Body = std::make_unique<CBody>(BodySpec);
		const glm::vec2 BodyPos = Body->GetPosition();
		TransformComp.Translation.x = BodyPos.x;
		TransformComp.Translation.y = BodyPos.y;
	}

	CActor::~CActor()
	{
		LK_DEBUG_TAG("Actor", "Delete: {}", Handle);
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

	FActorHandle CActor::GenerateHandle()
	{
		Instances++;
		return Instances;
	}

}