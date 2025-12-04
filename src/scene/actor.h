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

	class CScene;

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
		void SetSize(const glm::vec2& InSize);
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
		FORCEINLINE CBody* GetBody() { return Body ? Body.get() : nullptr; }
		FORCEINLINE const CBody* GetBody() const { return Body ? Body.get() : nullptr; }
		bool IsMoving() const;

		inline bool IsTickEnabled() const { return bTickEnabled; }
		void SetTickEnabled(bool Enabled);
		inline bool IsDeletable() const { return bDeletable; }
		void SetDeletable(bool Deletable);
		bool IsPlayer() const { return GetType() == EActorType::Player; }
		bool IsSensor() const { return (Body ? Body->IsSensor() : false); }

		inline ETexture GetTexture() const { return Texture; }
		inline const glm::vec4& GetColor() const { return Color; }
		void SetColor(const glm::vec4& InColor);

		inline std::string_view GetName() const { return Name; }
		void SetName(std::string_view InName);
		void SetTexture(ETexture InTexture);
		bool IsOutlineEnabled() const { return Outline.bEnabled; }
		float GetOutlineThickness() const { return Outline.Thickness; }
		const glm::vec4& GetOutlineColor() const { return Outline.Color; }
		void SetOutlineEnabled(bool Enabled);
		void SetOutlineThickness(float InThickness);
		void SetOutlineColor(const glm::vec4& InColor);

		virtual bool Serialize(YAML::Emitter& Out) const override;

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
		bool RemoveComponent()
		{
			/* Force generic template to be ill-formed, no return needed. */
			static_assert(sizeof(T) == 0, "RemoveComponent not specialized for this type");
		}

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
			if constexpr (!IsOneOf<FInteractionComponent, TExcluded...>)
			{
				Ret = (Ret || HasComponent<FEffectComponent>());
			}

			return Ret;
		}

	private:
		void UpdateEffectComponent(FEffectComponent& EC);

		template<typename T, typename... Ts>
		static constexpr bool IsOneOf = (std::is_same_v<T, Ts> || ...);

	protected:
		std::unique_ptr<CBody> Body;
		FTransformComponent TransformComp{};
		std::optional<FEffectComponent> EffectComp;
		std::optional<FInteractionComponent> InteractionComp;

		std::string Name;
		ETexture Texture = ETexture::White;
		glm::vec4 Color = FColor::White;

		struct FOutline
		{
			bool bEnabled = true;
			float Thickness = 0.0f;
			glm::vec4 Color = FColor::Transparent;
		};
		FOutline Outline;

	private:
		LUUID Handle;
		bool bTickEnabled = true;
		bool bDeletable = true;
	};

}

#include "actor_impl.h"
