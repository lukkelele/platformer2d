#pragma once

#include <cstdio>
#include <memory>
#include <utility>
#include <string>
#include <variant>

#include <glm/glm.hpp>
#include <glm/ext/matrix_common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/norm.hpp>

#include "core/core.h"
#include "core/enum.h"
#include "core/assert.h"
#include "core/log_formatters.h"
#include "game/itemtype.h"
#include "game/weapontype.h"

namespace platformer2d {

	class CCamera;

	struct FCameraComponent
	{
		std::shared_ptr<CCamera> Camera; /* @todo: Should be unique_ptr but need to sort out CCamera forward decl first. */
	};

	struct FTransformComponent
	{
	public:
		glm::vec3 Translation = {0.0f, 0.0f, 0.0f};
		glm::vec3 Scale = {1.0f, 1.0f, 1.0f};
		bool bIsStatic = false;

	private:
		glm::vec3 RotationEuler = {0.0f, 0.0f, 0.0f};
		glm::quat Rotation = {1.0f, 0.0f, 0.0f, 0.0f};

	public:
		FTransformComponent() = default;
		FTransformComponent(const glm::vec3& Translation)
			: Translation(Translation)
		{}
		FTransformComponent(const FTransformComponent& Other) = default;

		[[nodiscard]] glm::vec3 GetTranslation() const { return Translation; }
		[[nodiscard]] glm::vec3 GetScale() const { return Scale; }

		[[nodiscard]] glm::mat4 GetTransform() const
		{
			return glm::translate(glm::mat4(1.0f), Translation)
				* glm::toMat4(Rotation)
				* glm::scale(glm::mat4(1.0f), Scale);
		}

		[[nodiscard]] glm::mat4 GetInvTransform() const
		{
			const glm::mat4 InvTranslation = glm::translate(glm::mat4(1.0f), -Translation);
			const glm::quat InvRot = glm::conjugate(Rotation);
			const glm::mat4 InvScale = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f / Scale.x, 1.0f / Scale.y, 1.0f / Scale.z));
			return InvScale * glm::toMat4(InvRot) * InvTranslation;
		}

		[[nodiscard]] glm::quat GetRotation() const { return Rotation; }
		[[nodiscard]] glm::vec3 GetRotationEuler() const { return RotationEuler; }

		/**
		 * @brief Get 2D rotation in radians.
		 */
		[[nodiscard]] float GetRotation2D() const { return glm::eulerAngles(Rotation).z; }

		void SetTranslation(const glm::vec3& InTranslation) { Translation = InTranslation; }
		void SetTranslation(const glm::vec2& InTranslation) { Translation = glm::vec3(InTranslation, 0.0f); }

		void SetScale(const glm::vec3& InScale) { Scale = InScale; }
		void SetScale(const glm::vec2& InScale) { Scale = glm::vec3(InScale, 1.0f); }

		void SetRotationEuler(const glm::vec3& Euler)
		{
			RotationEuler = Euler;
			Rotation = glm::quat(RotationEuler);
		}

		void SetRotation(const glm::quat& InQuat)
		{
			Rotation = InQuat;

			const glm::vec3 OriginalEulerer = RotationEuler;
			RotationEuler = glm::eulerAngles(Rotation);

			/* Attempt to avoid 180deg flips in the Euler angles when using SetRotation(quat). */
			if ((std::fabs(RotationEuler.x - OriginalEulerer.x) == glm::pi<float>())
				&& (std::fabs(RotationEuler.z - OriginalEulerer.z) == glm::pi<float>())) {
				RotationEuler.x = OriginalEulerer.x;
				RotationEuler.y = glm::pi<float>() - RotationEuler.y;
				RotationEuler.z = OriginalEulerer.z;
			}
		}

		void SetRotation2D(const float Radians)
		{
			RotationEuler = glm::vec3(0.0f, 0.0f, Radians);
			Rotation = glm::quat(RotationEuler);
		}

		[[nodiscard]] bool IsStatic() const { return bIsStatic; }

		std::string ToString() const
		{
			return Format("Translation={} Scale={} RotEuler={}", Translation, Scale, RotationEuler);
		}
	};

	/**************************************
	 * EffectComponent
	 **************************************/
	enum class EEffectType : std::uint8_t
	{
		None,
		Rotate,
		COUNT
	};
	LK_ENUM(EEffectType);

	struct FRotateEffect
	{
		float AngularSpeedDegPerSecond = 0.0f;
	};

	using TEffectData = std::variant<std::monostate, FRotateEffect>;

	struct FEffectInstance
	{
		EEffectType Type = EEffectType::None;
		TEffectData Data;
	};

	struct FEffectComponent
	{
		std::vector<FEffectInstance> Effects;

		[[nodiscard]] bool HasAny() const { return !Effects.empty(); }
	};

	/**************************************
	 * InteractionComponent
	 **************************************/
	enum class EInteraction : std::uint16_t
	{
		None,
		Damage,
		Pickup,
		Heal,
		Killzone,
		Jumppad,
		Climbable,
		Checkpoint,
		COUNT
	};
	LK_ENUM(EInteraction);

	struct FDamageInteraction
	{
		float Damage = 0.0f;
	};

	struct FHealInteraction
	{
		float Amount = 25.0f;
		bool bConsumeOnUse = true;
	};

	struct FKillzoneInteraction
	{
	};

	struct FJumppadInteraction
	{
		glm::vec2 Impulse = {0.0f, 6.0f};
		bool bPreserveHorizontalVelocity = true;
	};

	struct FClimbableInteraction
	{
		float ClimbSpeed = 1.0f;
	};

	struct FCheckpointInteraction
	{
		std::string CheckpointID;
	};

	enum class EPickupKind : std::uint16_t
	{
		Item,
		Weapon,
		COUNT
	};
	LK_ENUM(EPickupKind);

	enum class EConsumableKind : std::uint16_t
	{
		None,
		Health,
		COUNT
	};
	LK_ENUM(EConsumableKind);

	enum class EItemPayload : std::uint16_t
	{
		None,
		Consumable,
		Ammo,
		COUNT
	};
	LK_ENUM(EItemPayload);

	struct FConsumablePayload
	{
		EConsumableKind Kind = EConsumableKind::Health;
		float Amount = 25.0f;
	};

	struct FAmmoPayload
	{
		EWeaponType Weapon = EWeaponType::Rifle;
		std::uint16_t Count = 30;
	};

	using TPickupItemPayload = std::variant<std::monostate, FConsumablePayload, FAmmoPayload>;

	struct FPickupItem
	{
		EItemType Type = EItemType::None;
		TPickupItemPayload Payload;
	};

	using TWeaponSpecification = std::variant<std::monostate, FRifleSpecification>;
	struct FPickupWeapon
	{
		EWeaponType Type;
		TWeaponSpecification Spec;
	};

	using TPickupObject = std::variant<std::monostate, FPickupItem, FPickupWeapon>;

	struct FPickupInteraction
	{
		EPickupKind Kind = EPickupKind::Item;
		TPickupObject Object;
		bool bExpireWhenPickedUp = false;
	};

	using TInteractionData = std::variant<
		std::monostate,
		FDamageInteraction,
		FPickupInteraction,
		FHealInteraction,
		FKillzoneInteraction,
		FJumppadInteraction,
		FClimbableInteraction,
		FCheckpointInteraction>;

	struct FInteractionComponent
	{
		EInteraction Type = EInteraction::None;
		TInteractionData Data;

		[[nodiscard]] EInteraction GetType() const { return Type; }
		[[nodiscard]] TInteractionData& GetData() { return Data; }
		[[nodiscard]] const TInteractionData& GetData() const { return Data; }
	};

	struct FHealthComponent
	{
		float MaxHealth = 100.0f;
		float Health = MaxHealth;
		bool bDamageable = true;

		[[nodiscard]] float GetHealth() const { return Health; }
		void SetHealth(const float InHealth)
		{
			Health = InHealth;
			if (Health < 0.0f) {
				Health = 0.0f;
			}
		}

		void SetMaxHealth() { Health = MaxHealth; }

		[[nodiscard]] float GetMaxHealth() const { return MaxHealth; }
		void SetMaxHealth(const float InMaxHealth) { MaxHealth = InMaxHealth; }

		[[nodiscard]] bool IsDamageable() const { return bDamageable; }
		void SetDamageable(const bool Enabled) { bDamageable = Enabled; }
		[[nodiscard]] bool IsDead() const { return (Health <= 0.0f); }
	};

}

