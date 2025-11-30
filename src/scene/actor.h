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

	class CScene;

	enum class EActorType : uint16_t
	{
		Object,
		Player,
		Spawnpoint,
	};

	enum EActorFlag : uint32_t
	{
		EActorFlag_None = 0,
		EActorFlag_Placeholder = LK_BIT(1),
	};

	class CActor : public ISerializable<ESerializable::Yaml>
	{
	public:
		CActor(const FActorSpecification& Spec = FActorSpecification());
		CActor(const FActorSpecification& InSpec, const FBodySpecification& BodySpec);
		virtual ~CActor();

		virtual void Tick(float DeltaTime);
		inline LUUID GetHandle() const { return Handle; }
		virtual EActorType GetType() const { return EActorType::Object; }

		glm::vec2 GetSize() const;
		glm::vec3 GetPosition() const;
		void SetPosition(float X, float Y);
		void SetPosition(const glm::vec2& NewPos);
		void SetPosition(const glm::vec3& NewPos);

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
		bool IsPlayer() const { return GetType() == EActorType::Player; }

		inline ETexture GetTexture() const { return Texture; }
		inline const glm::vec4& GetColor() const { return Color; }
		void SetColor(const glm::vec4& InColor);

		inline std::string_view GetName() const { return Name; }
		void SetName(std::string_view InName);
		void SetTexture(ETexture InTexture);
		float GetOutlineThickness() const { return Outline.Thickness; }
		const glm::vec4& GetOutlineColor() const { return Outline.Color; }
		void SetOutlineThickness(float InThickness);
		void SetOutlineColor(const glm::vec4& InColor);

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
		T* TryGetComponent()
		{
			/* Force generic template to be ill-formed, no return needed. */
			static_assert(sizeof(T) == 0, "GetComponent not specialized for this type");
		}

		template<typename T>
		const T* TryGetComponent() const
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

		template<typename... T>
		bool HasAny() const
		{
			return (HasComponent<T>() || ...);
		}

		template<typename... TExcluded>
		bool HasAnyExcept() const
		{
			bool Ret = false;
			if constexpr (!IsOneOf<FTransformComponent, TExcluded...>)
			{
				Ret = (Ret || HasComponent<FTransformComponent>());
			}
			if constexpr (!IsOneOf<FEffectComponent, TExcluded...>)
			{
				Ret = (Ret || HasComponent<FEffectComponent>());
			}

			return Ret;
		}

		virtual bool Serialize(YAML::Emitter& Out) const override;

	private:
		void UpdateEffectComponent(FEffectComponent& EC);

		template<typename T, typename... Ts>
		static constexpr bool IsOneOf = (std::is_same_v<T, Ts> || ...);

	protected:
		std::unique_ptr<CBody> Body;
		FTransformComponent TransformComp{};
		std::optional<FEffectComponent> EffectComp;

		std::string Name;
		ETexture Texture = ETexture::White;
		glm::vec4 Color = FColor::White;

		struct FOutline
		{
			float Thickness = 0.0f;
			glm::vec4 Color = FColor::Transparent;
		};
		FOutline Outline;

	private:
		LUUID Handle;
		bool bTickEnabled = true;
		bool bDeletable = true;
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
	inline FTransformComponent* CActor::TryGetComponent<FTransformComponent>()
	{
		return &TransformComp;
	}

	template<>
	inline const FTransformComponent* CActor::TryGetComponent<FTransformComponent>() const
	{
		return &TransformComp;
	}

	template<>
	inline FEffectComponent* CActor::TryGetComponent<FEffectComponent>()
	{
		if (!EffectComp.has_value())
		{
			return nullptr;
		}

		return std::addressof(EffectComp.value());
	}

	template<>
	inline const FEffectComponent* CActor::TryGetComponent<FEffectComponent>() const
	{
		if (!EffectComp.has_value())
		{
			return nullptr;
		}

		return std::addressof(EffectComp.value());
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
		LK_ASSERT(Value.HasAny(), "Added component has no effects");
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
