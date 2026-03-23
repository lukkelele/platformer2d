#pragma once
#include "actor.h"

#define LK_ASSERT_COMPONENT_RETRIEVAL 1
#if LK_ASSERT_COMPONENT_RETRIEVAL
#	define LK_ASSERT_GET_COMP(...) LK_ASSERT(__VA_ARGS__)
#else
#	define LK_ASSERT_GET_COMP(...)
#endif

namespace platformer2d {

	template<>
	inline FTransformComponent& CActor::AddComponent<FTransformComponent>()
	{
		return TransformComp;
	}

	template<>
	inline FTransformComponent& CActor::AddComponent<FTransformComponent>(const FTransformComponent& Other)
	{
		TransformComp = Other;
		return TransformComp;
	}

	template<>
	inline FEffectComponent& CActor::AddComponent<FEffectComponent>()
	{
		LK_DEBUG_TAG("Actor", "{}: Add effect component", Name);
		if (!EffectComp.has_value()) {
			EffectComp.emplace();
		}

		return EffectComp.value();
	}

	template<>
	inline FEffectComponent& CActor::AddComponent<FEffectComponent>(const FEffectComponent& Other)
	{
		LK_DEBUG_TAG("Actor", "{}: Add effect component", Name);
		if (!EffectComp.has_value()) {
			EffectComp = Other;
		} else {
			EffectComp.value() = Other;
		}

		return EffectComp.value();
	}

	template<>
	inline FInteractionComponent& CActor::AddComponent<FInteractionComponent>()
	{
		LK_DEBUG_TAG("Actor", "{}: Add interaction component", Name);
		if (!InteractionComp.has_value()) {
			InteractionComp.emplace();
		}

		return InteractionComp.value();
	}

	template<>
	inline FInteractionComponent& CActor::AddComponent<FInteractionComponent>(const FInteractionComponent& Other)
	{
		LK_DEBUG_TAG("Actor", "{}: Add interaction component", Name);
		if (!InteractionComp.has_value()) {
			InteractionComp = Other;
		} else {
			InteractionComp.value() = Other;
		}

		return InteractionComp.value();
	}

	template<>
	inline FHealthComponent& CActor::AddComponent<FHealthComponent>()
	{
		LK_DEBUG_TAG("Actor", "{}: Add health component", Name);
		if (!HealthComp.has_value()) {
			HealthComp.emplace();
		}

		return HealthComp.value();
	}

	template<>
	inline FHealthComponent& CActor::AddComponent<FHealthComponent>(const FHealthComponent& Other)
	{
		LK_DEBUG_TAG("Actor", "{}: Add health component", Name);
		if (!HealthComp.has_value()) {
			HealthComp = Other;
		} else {
			HealthComp.value() = Other;
		}

		return HealthComp.value();
	}

	template<>
	inline bool CActor::RemoveComponent<FTransformComponent>()
	{
		return false;
	}

	template<>
	inline bool CActor::RemoveComponent<FEffectComponent>()
	{
		LK_DEBUG_TAG("Actor", "{}: Remove effect component", Name);
		if (!EffectComp.has_value()) {
			return false;
		}

		EffectComp.reset();
		return true;
	}

	template<>
	inline bool CActor::RemoveComponent<FInteractionComponent>()
	{
		LK_DEBUG_TAG("Actor", "{}: Remove interaction component", Name);
		if (!InteractionComp.has_value()) {
			return false;
		}

		InteractionComp.reset();
		return true;
	}

	template<>
	inline bool CActor::RemoveComponent<FHealthComponent>()
	{
		LK_DEBUG_TAG("Actor", "{}: Remove health component", Name);
		if (!HealthComp.has_value()) {
			return false;
		}

		HealthComp.reset();
		return true;
	}

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
	inline FInteractionComponent& CActor::GetComponent<FInteractionComponent>()
	{
		LK_ASSERT_GET_COMP(InteractionComp.has_value());
		return InteractionComp.value();
	}

	template<>
	inline const FInteractionComponent& CActor::GetComponent<FInteractionComponent>() const
	{
		LK_ASSERT_GET_COMP(InteractionComp.has_value());
		return InteractionComp.value();
	}

	template<>
	inline FHealthComponent& CActor::GetComponent<FHealthComponent>()
	{
		LK_ASSERT_GET_COMP(HealthComp.has_value());
		return HealthComp.value();
	}

	template<>
	inline const FHealthComponent& CActor::GetComponent<FHealthComponent>() const
	{
		LK_ASSERT_GET_COMP(HealthComp.has_value());
		return HealthComp.value();
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
		return (EffectComp.has_value() ? std::addressof(EffectComp.value()) : nullptr);
	}

	template<>
	inline const FEffectComponent* CActor::TryGetComponent<FEffectComponent>() const
	{
		return (EffectComp.has_value() ? std::addressof(EffectComp.value()) : nullptr);
	}

	template<>
	inline FInteractionComponent* CActor::TryGetComponent<FInteractionComponent>()
	{
		return (InteractionComp.has_value() ? std::addressof(InteractionComp.value()) : nullptr);
	}

	template<>
	inline const FInteractionComponent* CActor::TryGetComponent<FInteractionComponent>() const
	{
		return (InteractionComp.has_value() ? std::addressof(InteractionComp.value()) : nullptr);
	}

	template<>
	inline FHealthComponent* CActor::TryGetComponent<FHealthComponent>()
	{
		return (HealthComp.has_value() ? std::addressof(HealthComp.value()) : nullptr);
	}

	template<>
	inline const FHealthComponent* CActor::TryGetComponent<FHealthComponent>() const
	{
		return (HealthComp.has_value() ? std::addressof(HealthComp.value()) : nullptr);
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

	template<>
	inline bool CActor::HasComponent<FInteractionComponent>() const
	{
		return InteractionComp.has_value();
	}

	template<>
	inline bool CActor::HasComponent<FHealthComponent>() const
	{
		return HealthComp.has_value();
	}

}

#undef LK_ASSERT_GET_COMP
