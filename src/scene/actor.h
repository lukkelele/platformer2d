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
	class CSprite;

	enum class EMovementState
	{
		Idle,
		Running,
		Airborne,
		COUNT
	};
	LK_ENUM(EMovementState);

	enum class EDeathReason
	{
		Unknown,
		KillCommand,
		Killzone,
		Damage,
		COUNT
	};
	LK_ENUM(EDeathReason);

	class CActor : public LObject, public ISerializable<ESerializable::Yaml>
	{
	public:
		CActor(const FActorSpecification& Spec = FActorSpecification());
		CActor(const FActorSpecification& InSpec, const FBodySpecification& BodySpec);
		CActor(CActor&&) = delete;
		CActor(const CActor&) = delete;
		virtual ~CActor();

		CActor& operator=(CActor&&) = delete;
		CActor& operator=(const CActor&) = delete;

		virtual void Tick(float DeltaTime);
		[[nodiscard]] LUUID GetHandle() const { return Handle; }
		[[nodiscard]] virtual EActorType GetActorType() const { return EActorType::Object; }

		virtual void OnDeath(EDeathReason Reason) {}

		template<typename T>
		T& As()
		{
			static_assert(sizeof(T) > 0, "As<T> failed, incomplete type");
			return static_cast<T&>(*this);
		}

		template<typename T>
		const T& As() const
		{
			static_assert(sizeof(T) > 0, "As<T> failed, incomplete type");
			return static_cast<const T&>(*this);
		}

		[[nodiscard]] std::underlying_type_t<EActorFlag> GetFlags() const { return ActorFlags; }

		[[nodiscard]] bool HasFlag(const EActorFlag Flag) const
		{
			return static_cast<bool>(ActorFlags & std::to_underlying(Flag));
		}

		void SetFlag(const EActorFlag Flag, bool Value = true)
		{
			if (Value) {
				ActorFlags |= Flag;
			} else {
				ActorFlags &= ~Flag;
			}
		}

		[[nodiscard]] glm::vec2 GetSize() const;
		void SetSize(const glm::vec2& InSize);
		[[nodiscard]] const glm::vec3& GetPosition() const { return TransformComp.Translation; }
		void SetPosition(float X, float Y);
		void SetPosition(const glm::vec2& NewPos);
		void SetPosition(const glm::vec3& NewPos);

		/**
		 * @brief Get rotation in radians.
		 */
		float GetRotation() const;
		void SetRotation(float AngleRad);

		void SetScale(const glm::vec2& NewScale);

		[[nodiscard]] FTransformComponent& GetTransformComponent() { return TransformComp; }
		[[nodiscard]] const FTransformComponent& GetTransformComponent() const { return TransformComp; }
		[[nodiscard]] CBody* GetBody() { return Body ? Body.get() : nullptr; }
		[[nodiscard]] const CBody* GetBody() const { return Body ? Body.get() : nullptr; }
		void ReplaceBody(const FBodySpecification& NewSpec);
		[[nodiscard]] bool IsMoving() const;

		[[nodiscard]] bool IsTickEnabled() const { return bTickEnabled; }
		void SetTickEnabled(bool Enabled);
		[[nodiscard]] bool IsDeletable() const { return bDeletable; }
		void SetDeletable(bool Deletable);
		[[nodiscard]] bool IsPlayer() const { return GetActorType() == EActorType::Player; }
		[[nodiscard]] bool IsSensor() const { return (Body ? Body->IsSensor() : false); }

		[[nodiscard]] ETexture GetTexture() const { return Texture; }
		[[nodiscard]] const glm::vec4& GetColor() const { return Color; }
		void SetColor(const glm::vec4& InColor);

		[[nodiscard]] const glm::vec2& GetSpriteScale() const { return SpriteScale; }
		void SetSpriteScale(const glm::vec2& InScale) { SpriteScale = InScale; }

		[[nodiscard]] virtual const CSprite* GetSprite() const { return nullptr; }

		[[nodiscard]] std::string_view GetName() const { return Name; }
		void SetName(std::string_view InName);
		void SetTexture(ETexture InTexture);
		[[nodiscard]] bool IsOutlineEnabled() const { return Outline.bEnabled; }
		[[nodiscard]] float GetOutlineThickness() const { return Outline.Thickness; }
		[[nodiscard]] const glm::vec4& GetOutlineColor() const { return Outline.Color; }
		void SetOutlineEnabled(bool Enabled);
		void SetOutlineThickness(float InThickness);
		void SetOutlineColor(const glm::vec4& InColor);

		virtual bool Serialize(YAML::Emitter& Out, EExtendableSerializer Extendable = EExtendableSerializer::No) const override;

		template<typename T>
		T& AddComponent()
		{
			static_assert(sizeof(T) == 0, "AddComponent not specialized for this type");
		}

		template<typename T>
		T& AddComponent(const T& Value)
		{
			static_assert(!std::is_same_v<T, T>, "AddComponent not specialized for this type");
		}

		template<typename T>
		bool RemoveComponent()
		{
			static_assert(sizeof(T) == 0, "RemoveComponent not specialized for this type");
			return false;
		}

		template<typename T>
		[[nodiscard]] T& GetComponent()
		{
			static_assert(sizeof(T) == 0, "GetComponent not specialized for this type");
		}

		template<typename T>
		[[nodiscard]] const T& GetComponent() const
		{
			static_assert(sizeof(T) == 0, "GetComponent not specialized for this type");
		}

		template<typename T>
		[[nodiscard]] T* TryGetComponent()
		{
			static_assert(sizeof(T) == 0, "GetComponent not specialized for this type");
		}

		template<typename T>
		[[nodiscard]] const T* TryGetComponent() const
		{
			static_assert(sizeof(T) == 0, "GetComponent not specialized for this type");
		}

		template<typename T>
		[[nodiscard]] bool HasComponent() const
		{
			return false;
		}

		template<typename... T>
		[[nodiscard]] bool HasAnyComponents() const
		{
			return (HasComponent<T>() || ...);
		}

		template<typename... TExcluded>
		[[nodiscard]] bool HasAnyComponentsExcept() const
		{
			bool Ret = false;
			if constexpr (!IsComponentOneOf<FTransformComponent, TExcluded...>) {
				Ret = (Ret || HasComponent<FTransformComponent>());
			}
			if constexpr (!IsComponentOneOf<FEffectComponent, TExcluded...>) {
				Ret = (Ret || HasComponent<FEffectComponent>());
			}
			if constexpr (!IsComponentOneOf<FInteractionComponent, TExcluded...>) {
				Ret = (Ret || HasComponent<FEffectComponent>());
			}

			return Ret;
		}

	private:
		void UpdateEffectComponent(FEffectComponent& EC);

		template<typename T, typename... Ts>
		static constexpr bool IsComponentOneOf = (std::is_same_v<T, Ts> || ...);

	protected:
		std::unique_ptr<CBody> Body;
		FTransformComponent TransformComp{};
		std::optional<FEffectComponent> EffectComp;
		std::optional<FInteractionComponent> InteractionComp;
		std::optional<FHealthComponent> HealthComp;
		std::optional<FCameraComponent> CameraComp;
		std::optional<FCombatComponent> CombatComp;

		std::string Name;
		std::underlying_type_t<EActorFlag> ActorFlags = EActorFlag_None;
		ETexture Texture = ETexture::White;
		glm::vec4 Color = FColor::White;
		glm::vec2 SpriteScale = {1.0f, 1.0f};

		struct FOutline
		{
			bool bEnabled = true;
			float Thickness = 0.0f;
			glm::vec4 Color = FColor::Black;
		};
		FOutline Outline;

	private:
		LUUID Handle;
		bool bTickEnabled = true;
		bool bDeletable = true;

		LK_CLASS();
	};

}

#include "actor_impl.h"

