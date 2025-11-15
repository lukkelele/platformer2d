#pragma once

#include "actorspecification.h"
#include "core/core.h"
#include "core/assert.h"
#include "core/delegate.h"
#include "components.h"
#include "renderer/color.h"
#include "renderer/texture.h"
#include "physics/body.h"
#include "serialization/serializable.h"

namespace platformer2d {

	using FActorHandle = uint32_t;

	class CActor : public ISerializable
	{
	public:
		LK_DECLARE_EVENT(FOnActorCreated, CActor, FActorHandle, std::weak_ptr<CActor>);
		LK_DECLARE_MULTICAST_DELEGATE(FOnActorMarkedForDeletion, FActorHandle);
	public:
		CActor(const FActorSpecification& Spec = FActorSpecification());
		CActor(FActorHandle InHandle, const FBodySpecification& BodySpec, ETexture InTexture = ETexture::White, const glm::vec4& InColor = FColor::White);
		CActor(const FBodySpecification& BodySpec, ETexture InTexture = ETexture::White, const glm::vec4& InColor = FColor::White);
		virtual ~CActor();

		template<typename T, typename... TArgs>
		static std::shared_ptr<T> Create(TArgs&&... Args)
		{
			static_assert(std::is_base_of_v<CActor, T>);
			std::shared_ptr<T> Actor = std::shared_ptr<T>(new T(std::forward<TArgs>(Args)...));
			Instances++;
			CActor::OnActorCreated.Broadcast(Actor->GetHandle(), std::weak_ptr<CActor>(Actor));
			return Actor;
		}

		virtual void Tick(float DeltaTime);
		inline FActorHandle GetHandle() const { return Handle; }

		glm::vec2 GetPosition() const;
		void SetPosition(float X, float Y);
		void SetPosition(const glm::vec2& NewPos);

		/**
		 * @brief Get rotation in radians.
		 */
		float GetRotation() const;
		void SetRotation(float AngleRad);

		inline FTransformComponent& GetTransformComponent() { return TransformComp; }
		inline const FTransformComponent& GetTransformComponent() const { return TransformComp; }
		inline CBody& GetBody() { return *Body; }
		inline const CBody& GetBody() const { return *Body; }
		bool IsMoving() const;

		inline bool IsTickEnabled() const { return bTickEnabled; }
		void SetTickEnabled(bool Enabled);
		inline bool IsDeletable() const { return bDeletable; }
		void SetDeletable(bool Deletable);

		inline ETexture GetTexture() const { return Texture; }
		inline const glm::vec4& GetColor() const { return Color; }
		void SetColor(const glm::vec4& InColor);

		inline std::string_view GetName() const { return Name; }

		virtual void Serialize(YAML::Emitter& Out) override;

	private:
		static FActorHandle GenerateHandle();

	public:
		static inline FOnActorCreated OnActorCreated;
		static inline FOnActorMarkedForDeletion OnActorMarkedForDeletion;
	protected:
		FTransformComponent TransformComp{};
		std::unique_ptr<CBody> Body;
		ETexture Texture = ETexture::White;
		glm::vec4 Color = FColor::White;
		std::string Name;

	private:
		FActorHandle Handle;
		bool bTickEnabled = true;
		bool bDeletable = true;

		static inline uint32_t Instances = 0;
		static_assert(std::is_same_v<FActorHandle, decltype(Instances)>);
	};

}
