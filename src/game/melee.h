#pragma once

#include <chrono>
#include <vector>

#include "weapon.h"

namespace platformer2d {

	enum class EMeleeState : std::uint8_t
	{
		Idle,
		Windup,
		Active,
		Recovery,
	};

	class CMelee : public IWeapon
	{
	public:
		CMelee(const FMeleeSpecification& InSpec = FMeleeSpecification(), CActor* InOwner = nullptr);
		~CMelee() override = default;

		void Tick(float DeltaTime) override;
		void Render() override;
		[[nodiscard]] EWeaponType GetWeaponType() const override { return EWeaponType::Melee; }

		void PrimaryAction(const glm::vec2& TargetWorldPos) override;

		void Equip(CActor* Actor);
		[[nodiscard]] bool IsHeldBy(const CActor* Actor) const { return (Actor && (Owner == Actor)); }
		[[nodiscard]] const CActor* GetOwner() const { return Owner; }

		bool Swing();

		void SetLookDirection(EDirection InDirection) { LookDir = InDirection; }
		[[nodiscard]] EDirection GetLookDirection() const { return LookDir; }
		[[nodiscard]] EMeleeState GetState() const { return State; }
		[[nodiscard]] bool IsBusy() const { return State != EMeleeState::Idle; }

		void SetDamage(const float InDamage) { Spec.Damage = InDamage; }
		[[nodiscard]] float GetDamage() const { return Spec.Damage; }
		void SetReach(const float InReach) { Spec.Reach = InReach; }
		[[nodiscard]] float GetReach() const { return Spec.Reach; }

	private:
		void TickSwing(float DeltaTime);
		void QueryAndDamage();
		[[nodiscard]] float GetSwingProgress() const;
		[[nodiscard]] static bool OnOverlapStatic(b2ShapeId ShapeId, void* Context);
		[[nodiscard]] bool OnOverlap(b2ShapeId ShapeId);

	private:
		CActor* Owner = nullptr;
		FMeleeSpecification Spec;
		EDirection LookDir = EDirection::Right;

		EMeleeState State = EMeleeState::Idle;
		std::chrono::steady_clock::time_point StateEntered;

		std::vector<CActor*> AlreadyHit;
	};

}
