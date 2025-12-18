#pragma once

#include "core/core.h"
#include "physics/body.h"
#include "scene/components.h"
#include "yaml.h"

namespace platformer2d::Serialization {

	/**
	 * @def LK_DESERIALIZE_PROPERTY
	 * @brief Get a value from a node with a fallback value incase it doesn't exist.
	 * Cannot be used with strings as of yet.
	 */
	#define LK_DESERIALIZE_PROPERTY(PropertyName, Destination, DefaultValue, Node) \
		if (Node.IsDefined()) { \
			if (auto NodeRef = Node[#PropertyName]) { \
				try { \
					Destination = NodeRef.as<decltype(DefaultValue)>(); \
				} catch (const std::exception& Exception) { \
					LK_ERROR_TAG("Deserializer", "Failed to get \"{}\": {}", #PropertyName, Exception.what()); \
					Destination = DefaultValue; \
				} \
			} else { \
				LK_WARN_TAG("Deserializer", "Property not found: {}", #PropertyName); \
			} \
		} else { \
			LK_ERROR_TAG("Deserializer", "Default value used for: {}", #PropertyName); \
			Destination = DefaultValue; \
		}

	template<typename T>
	static void Serialize(const T& Target, YAML::Emitter& Out)
	{
		LK_UNUSED(Target, Out);
	}

	template<>
	inline void Serialize(const FTransformComponent& TC, YAML::Emitter& Out)
	{
		Out << YAML::Key << "TransformComponent";
		Out << YAML::BeginMap;
		Out << YAML::Key << "Position" << YAML::Value << TC.Translation;
		Out << YAML::Key << "Rotation" << YAML::Value << TC.GetRotation2D();
		Out << YAML::Key << "Scale" << YAML::Value << TC.Scale;
		Out << YAML::EndMap;
	}

	template<>
	inline void Serialize(const FEffectComponent& EC, YAML::Emitter& Out)
	{
		if (EC.Effects.empty()) {
			LK_DEBUG_TAG("Serializer", "EffectComponent has no effects");
			return;
		}

		Out << YAML::Key << "EffectComponent";
		Out << YAML::Value << YAML::BeginSeq;
		for (std::size_t Index = 0; Index < EC.Effects.size(); Index++) {
			const FEffectInstance& Instance = EC.Effects[Index];
			YAML::Node EffectNode;

			Out << YAML::BeginMap;
			Out << YAML::Key << "Type" << YAML::Value << std::to_underlying(Instance.Type);

			switch (Instance.Type) {
				case EEffectType::Rotate: {
					const FRotateEffect& Rotate = std::get<FRotateEffect>(Instance.Data);
					Out << YAML::Key << "AngularSpeedDegPerSecond" << YAML::Value << Rotate.AngularSpeedDegPerSecond;
					break;
				}
				default:
					break;
			}

			Out << YAML::EndMap;
		}

		Out << YAML::EndSeq;
	}

	template<>
	inline void Serialize(const FInteractionComponent& IC, YAML::Emitter& Out)
	{
		Out << YAML::Key << "InteractionComponent";
		Out << YAML::BeginMap;
		Out << YAML::Key << "InteractionType" << YAML::Value << std::to_underlying(IC.GetType());
		switch (IC.Type) {
			case EInteraction::Damage: {
				const auto& Data = std::get<FDamageInteraction>(IC.Data);
				Out << YAML::Key << "Damage" << YAML::Value << Data.Damage;
				break;
			}
			case EInteraction::Pickup: {
				const auto& Data = std::get<FPickupInteraction>(IC.Data);
				Out << YAML::Key << "PickupKind" << YAML::Value << std::to_underlying(Data.Kind);

				Out << YAML::Key << "Object";
				Out << YAML::BeginMap;
				int ObjectType = -1;
				if (const FPickupItem* Object = std::get_if<FPickupItem>(&Data.Object)) {
					ObjectType = static_cast<int>(Object->Type);
					LK_FATAL_TAG("Serializer", "PickupItem ObjectType={}", ObjectType);
				} else if (const FPickupWeapon* Object = std::get_if<FPickupWeapon>(&Data.Object)) {
					ObjectType = static_cast<int>(Object->Type);
					LK_DEBUG_TAG("Serializer", "PickupWeapon ObjectType={}", ObjectType);
					if (Object->Type == EWeaponType::Rifle) {
						auto& Spec = std::get<FRifleSpecification>(Object->Spec);
						Out << YAML::Key << "Specification";
						Out << YAML::BeginMap;
						Out << YAML::Key << "MagazineSize" << YAML::Value << Spec.MagazineSize;
						Out << YAML::EndMap;
					}
				} else {
					LK_ERROR_TAG("Serializer", "Object data missing for {}", Enum::ToString(Data.Kind));
				}
				Out << YAML::EndMap; /* Object */
				Out << YAML::Key << "ObjectType" << YAML::Value << ObjectType;

				Out << YAML::Key << "ExpireWhenPickedUp" << YAML::Value << Data.bExpireWhenPickedUp;
				break;
			}
			default:
				LK_ERROR_TAG("Serializer", "Interaction {} not supported", Enum::ToString(IC.Type));
				break;
		}

		Out << YAML::EndMap;
	}

	template<typename T>
	static void Deserialize(T& Target, const YAML::Node& Node)
	{
		LK_UNUSED(Target, Node);
	}

	template<>
	inline void Deserialize(FTransformComponent& TC, const YAML::Node& Node)
	{
		TC.Translation = Node["Position"].as<decltype(TC.Translation)>();
		const float RotRad = Node["Rotation"].as<float>();
		TC.SetRotation2D(RotRad);
		TC.Scale = Node["Scale"].as<decltype(TC.Scale)>();
	}

	/**
	 * @brief Deserialize an effect component.
	 * The YAML node should be a sequence of effects, if any.
	 */
	template<>
	void Deserialize(FEffectComponent& EC, const YAML::Node& NodeSeq)
	{
		LK_ASSERT(NodeSeq && NodeSeq.IsSequence());
		for (std::size_t Idx = 0; Idx < NodeSeq.size(); Idx++) {
			const YAML::Node& EffectNode = NodeSeq[Idx];
			if (!EffectNode) {
				continue;
			}

			FEffectInstance Instance;
			Instance.Type = EEffectType::None;
			Instance.Data = std::monostate{};

			const EEffectType Type = static_cast<EEffectType>(EffectNode["Type"].as<std::size_t>());
			if (Type == EEffectType::Rotate) {
				Instance.Type = EEffectType::Rotate;
				FRotateEffect Rotate;
				Rotate.AngularSpeedDegPerSecond = EffectNode["AngularSpeedDegPerSecond"].as<float>();
				Instance.Data = Rotate;
			}

			if (Instance.Type != EEffectType::None) {
				LK_TRACE("Push effect: {}", Enum::ToString(Instance.Type));
				EC.Effects.push_back(Instance);
			}
		}
	}

	template<>
	inline void Deserialize(FInteractionComponent& IC, const YAML::Node& Node)
	{
		IC.Type = static_cast<EInteraction>(Node["InteractionType"].as<std::underlying_type_t<EInteraction>>());
		switch (IC.Type) {
			case EInteraction::Damage: {
				FDamageInteraction Data;
				LK_DESERIALIZE_PROPERTY(Damage, Data.Damage, 0.0f, Node);
				IC.Data = Data;
				LK_DEBUG_TAG("Deserializer", "FDamageInteraction::Damage: {}", Data.Damage);
				break;
			}
			case EInteraction::Pickup: {
				FPickupInteraction Data;
				LK_DESERIALIZE_PROPERTY(PickupKind, Data.Kind, EPickupKind::Item, Node);
				LK_DESERIALIZE_PROPERTY(ExpireWhenPickedUp, Data.bExpireWhenPickedUp, false, Node);

				int ObjectTypeValue = 0;
				try {
					/* The actual enum value to the type based on the kind. */
					ObjectTypeValue = Node["ObjectType"].as<int>();
				} catch (const std::exception& E) {
					LK_ERROR_TAG("Deserializer", "Failed to deserialize 'Object'");
					Data.Object = std::monostate{};
					ObjectTypeValue = -1;
				}

				if (ObjectTypeValue >= 0) {
					if (Data.Kind == EPickupKind::Item) {
						FPickupItem Object = {
							.Type = static_cast<EItemType>(ObjectTypeValue)
						};
						Data.Object = Object;
					} else if (Data.Kind == EPickupKind::Weapon) {
						FPickupWeapon Object = {
							.Type = static_cast<EWeaponType>(ObjectTypeValue)
						};

						const YAML::Node& ObjectNode = Node["Object"];
						LK_ASSERT(ObjectNode.IsDefined(), "Object node not defined");
						if (Object.Type == EWeaponType::Rifle) {
							const YAML::Node& SpecNode = ObjectNode["Specification"];
							FRifleSpecification Spec;
							LK_DESERIALIZE_PROPERTY(MagazineSize, Spec.MagazineSize, 30, SpecNode);
							Object.Spec = Spec;
							Data.Object = Object;
						}
					}
				} else {
					LK_ERROR_TAG("Deserializer", "Failed to deduce object type from {}", Enum::ToString(Data.Kind));
				}

				IC.Data = Data;
				LK_DEBUG_TAG("Deserializer", "FPickupInteraction: Kind={} bExpireWhenPickedUp={} ObjectTypeValue={}", Enum::ToString(Data.Kind), Data.bExpireWhenPickedUp, ObjectTypeValue);
				break;
			}
			default:
				LK_ERROR_TAG("Deserializer", "Interaction {} not supported", Enum::ToString(IC.Type));
				break;
		}
	}

	template<>
	inline void Deserialize(FBodySpecification& BodySpec, const YAML::Node& Node)
	{
		LK_ASSERT(Node["Type"] && Node["Shape"]);
		BodySpec.Type = static_cast<EBodyType>(Node["Type"].as<int>());

		LK_DESERIALIZE_PROPERTY(GravityScale, BodySpec.GravityScale, 1.0f, Node);

		const YAML::Node ShapeNode = Node["Shape"];
		LK_VERIFY(ShapeNode["ShapeType"], "ShapeType missing in yaml");
		const EShape ShapeType = static_cast<EShape>(ShapeNode["ShapeType"].as<int>());
		switch (ShapeType) {
			case EShape::Polygon: {
				const glm::vec2 Size = ShapeNode["Size"].as<glm::vec2>();
				const float Rotation = ShapeNode["Rotation"].as<float>();
				const float Radius = ShapeNode["Radius"].as<float>();
				LK_TRACE("Deserialize: Polygon: Size={} Rotation={} Radius={}", Size, Rotation, Radius);

				FPolygon Polygon = {
					.Size = Size,
					.Radius = Radius,
					.Rotation = Rotation,
				};
				BodySpec.Shape.emplace<FPolygon>(Polygon);
				break;
			}
			case EShape::Line: {
				LK_MARK_NOT_IMPLEMENTED();
				break;
			}
			case EShape::Capsule: {
				const glm::vec2 P0 = ShapeNode["Size"].as<glm::vec2>();
				const glm::vec2 P1 = ShapeNode["Size"].as<glm::vec2>();
				const float Radius = ShapeNode["Radius"].as<float>();
				LK_DEBUG("Deserialize: Capsule: P0={} P1={} Radius={}", P0, P1, Radius);

				FCapsule Capsule = {
					.P0 = P0,
					.P1 = P1,
					.Radius = Radius,
				};
				BodySpec.Shape.emplace<FCapsule>(Capsule);
				break;
			}
		}

		using PosType = decltype(BodySpec.Position);
		BodySpec.Position = Node["Position"].as<PosType>();

		using FlagsType = std::underlying_type_t<EBodyFlag>;
		BodySpec.Flags = Node["Flags"].as<FlagsType>();

		BodySpec.Density = Node["Mass"].as<float>(); /** @todo Density <-> Mass, equivalent in terms of body creation? */
		BodySpec.MotionLock = Node["MotionLock"].as<std::underlying_type_t<EMotionLock>>();

		LK_DESERIALIZE_PROPERTY(Sensor, BodySpec.bSensor, false, Node);
	}

}
