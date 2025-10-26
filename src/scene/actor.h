#pragma once

#include "core/core.h"
#include "core/assert.h"
#include "core/delegate.h"
#include "components.h"
#include "physics/body.h"
#include "actorspecification.h"

namespace platformer2d {

	using FActorHandle = uint64_t;

	class CActor
	{
	public:
		LK_DECLARE_EVENT(FOnActorCreated, CActor, FActorHandle, std::weak_ptr<CActor>);
	public:
		CActor(const FActorSpecification& Spec = FActorSpecification());
		CActor(const FBodySpecification& BodySpec);
		virtual ~CActor();

		template<typename T, typename... TArgs>
		static std::shared_ptr<T> Create(TArgs&&... Args)
		{
			static_assert(std::is_base_of_v<CActor, T>);
			std::shared_ptr<T> Actor = std::shared_ptr<T>(new T(std::forward<TArgs>(Args)...));
			CActor::OnActorCreated.Broadcast(Actor->GetHandle(), std::weak_ptr<CActor>(Actor));
			return Actor;
		}

		virtual void Tick(float DeltaTime);
		inline FActorHandle GetHandle() const { return Handle; }

		glm::vec2 GetPosition() const;
		void SetPosition(float X, float Y);
		void SetPosition(const glm::vec2& NewPos);

		inline FTransformComponent& GetTransformComponent() { return TransformComp; }
		inline const FTransformComponent& GetTransformComponent() const { return TransformComp; }
		inline const CBody& GetBody() const { return *Body; }

	private:
		static FActorHandle GenerateHandle();

	public:
		static inline FOnActorCreated OnActorCreated;
	protected:
		FTransformComponent TransformComp{};
		std::unique_ptr<CBody> Body;

	private:
		FActorHandle Handle;

		static inline uint64_t Instances = 0;
		static_assert(std::is_same_v<FActorHandle, decltype(Instances)>);
	};

}
