#pragma once

#include "core/core.h"
#include "core/assert.h"
#include "components.h"
#include "physics/body.h"
#include "actorspecification.h"

namespace platformer2d {

	class CActor
	{
	public:
		CActor(const FActorSpecification& Spec = FActorSpecification());
		CActor(const FBodySpecification& BodySpec);
		virtual ~CActor() = default;

		virtual void Tick(float DeltaTime);

		glm::vec2 GetPosition() const;
		void SetPosition(float X, float Y);
		void SetPosition(const glm::vec2& NewPos);

		inline FTransformComponent& GetTransformComponent() { return TransformComp; }
		inline const FTransformComponent& GetTransformComponent() const { return TransformComp; }
		inline const CBody& GetBody() const { return *Body; }

	protected:
		FTransformComponent TransformComp{};
		std::unique_ptr<CBody> Body;
	};

}
