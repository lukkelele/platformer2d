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

#define LK_ASSERT_COMPONENT_RETRIEVAL 1
#if LK_ASSERT_COMPONENT_RETRIEVAL
#	define LK_ASSERT_GET_COMP(...) LK_ASSERT(__VA_ARGS__)
#else
#	define LK_ASSERT_GET_COMP(...)
#endif

namespace platformer2d {

	class CActor : public ISerializable<ESerializable::Yaml>
	{
	public:
		LK_DECLARE_EVENT(FOnActorCreated, CActor, LUUID, std::weak_ptr<CActor>);
		LK_DECLARE_MULTICAST_DELEGATE(FOnActorMarkedForDeletion, LUUID);
	public:
		CActor(const FActorSpecification& Spec = FActorSpecification());
		CActor(LUUID InHandle, const FBodySpecification& BodySpec, ETexture InTexture = ETexture::White, const glm::vec4& InColor = FColor::White);
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
		inline LUUID GetHandle() const { return Handle; }

		glm::vec2 GetSize() const;
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
		FORCEINLINE CBody& GetBody() { return *Body; }
		FORCEINLINE const CBody& GetBody() const { return *Body; }
		bool IsMoving() const;

		inline bool IsTickEnabled() const { return bTickEnabled; }
		void SetTickEnabled(bool Enabled);
		inline bool IsDeletable() const { return bDeletable; }
		void SetDeletable(bool Deletable);

		inline ETexture GetTexture() const { return Texture; }
		inline const glm::vec4& GetColor() const { return Color; }
		void SetColor(const glm::vec4& InColor);

		inline std::string_view GetName() const { return Name; }

		template<typename T>
		T& GetComponent()
		{
			/* Force generic template to be ill-formed, no return needed. */
			static_assert(sizeof(T) == 0, "GetComponent not specialized for this type");
		}

		template<typename T>
		const T& GetComponent() const
		{
			/* Force generic template to be ill-formed, no return needed. */
			static_assert(sizeof(T) == 0, "GetComponent not specialized for this type");
		}

		template<typename T>
		T& AddComponent()
		{
			/* Force generic template to be ill-formed, no return needed. */
			static_assert(sizeof(T) == 0, "AddComponent not specialized for this type");
		}

		template<typename T>
		T& AddComponent(const T& Value)
		{
			/* Force generic template to be ill-formed, no return needed. */
			static_assert(!std::is_same_v<T, T>, "AddComponent not specialized for this type");
		}

		template<typename T>
		bool HasComponent() const
		{
			return false;
		}

		virtual bool Serialize(YAML::Emitter& Out) const override;

	private:
		static LUUID GenerateHandle();

	public:
		static inline FOnActorCreated OnActorCreated;
		static inline FOnActorMarkedForDeletion OnActorMarkedForDeletion;
	protected:
		std::unique_ptr<CBody> Body;
		FTransformComponent TransformComp{};
		std::optional<FEffectComponent> EffectComp;

		ETexture Texture = ETexture::White;
		glm::vec4 Color = FColor::White;
		std::string Name;
	private:
		LUUID Handle;
		bool bTickEnabled = true;
		bool bDeletable = true;

		static inline uint32_t Instances = 0;

		friend class CScene;
	};

	template<>
	inline FTransformComponent& CActor::GetComponent<FTransformComponent>()
	{
		return TransformComp;
	}

	template<>
	inline const FTransformComponent& CActor::GetComponent<FTransformComponent>() const
	{
		return TransformComp;
	}

	template<>
	inline FEffectComponent& CActor::GetComponent<FEffectComponent>()
	{
		LK_ASSERT_GET_COMP(EffectComp.has_value());
		return EffectComp.value();
	}

	template<>
	inline const FEffectComponent& CActor::GetComponent<FEffectComponent>() const
	{
		LK_ASSERT_GET_COMP(EffectComp.has_value());
		return EffectComp.value();
	}

	template<>
	inline FEffectComponent& CActor::AddComponent<FEffectComponent>()
	{
		if (!EffectComp.has_value())
		{
			EffectComp.emplace();
		}

		return EffectComp.value();
	}

	template<>
	inline FEffectComponent& CActor::AddComponent<FEffectComponent>(const FEffectComponent& Value)
	{
		if (!EffectComp.has_value())
		{
			EffectComp = Value;
		}
		else
		{
			EffectComp.value() = Value;
		}

		return EffectComp.value();
	}

	template<>
	inline bool CActor::HasComponent<FTransformComponent>() const
	{
		return true;
	}

	template<>
	inline bool CActor::HasComponent<FEffectComponent>() const
	{
		return EffectComp.has_value();
	}

}

#undef LK_ASSERT_GET_COMP
