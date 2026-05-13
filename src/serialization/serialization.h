#pragma once

#include <fstream>

#include "core/core.h"
#include "core/math/math.h"
#include "physics/body.h"
#include "scene/components.h"
#include "yaml.h"

namespace platformer2d::Serialization {

	enum EProperty : std::uint8_t
	{
		Required,
		Optional,
	};

	template<EProperty PropertyOption = EProperty::Required, typename TDest, typename TDefault>
	static void DeserializeProperty(std::string_view PropertyName, TDest& Destination, const TDefault& DefaultValue, const YAML::Node& Node)
	{
		if (!Node.IsDefined()) {
			LK_ERROR_TAG("Deserializer", R"(Default value used for: "{}")", PropertyName);
			Destination = static_cast<TDest>(DefaultValue);
			return;
		}

		const YAML::Node& NodeRef = Node[PropertyName];
		if (!NodeRef.IsDefined()) {
			if constexpr (PropertyOption == EProperty::Required) {
				LK_ERROR_TAG("Deserializer", R"(Property not found: "{}")", PropertyName);
			} else {
				LK_TRACE_TAG("Deserializer", R"(Property not found: "{}")", PropertyName);
			}
			return;
		}

		try {
			Destination = NodeRef.as<std::remove_cvref_t<TDefault>>();
		} catch (const std::exception& Exception) {
			LK_ERROR_TAG("Deserializer", R"(Failed to get "{}": {})", PropertyName, Exception.what());
			Destination = static_cast<TDest>(DefaultValue);
		}
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
		Out << YAML::Key << "Position" << YAML::Value << glm::vec3(Math::Round(TC.Translation.x), Math::Round(TC.Translation.y), Math::Round(TC.Translation.z));
		Out << YAML::Key << "Rotation" << YAML::Value << Math::Round(TC.GetRotation2D());
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
				case EEffectType::Rotate:
				{
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
			case EInteraction::Damage:
			{
				const auto& Data = std::get<FDamageInteraction>(IC.Data);
				Out << YAML::Key << "Damage" << YAML::Value << Data.Damage;
				break;
			}
			case EInteraction::Pickup:
			{
				const auto& Data = std::get<FPickupInteraction>(IC.Data);
				Out << YAML::Key << "PickupKind" << YAML::Value << std::to_underlying(Data.Kind);

				Out << YAML::Key << "Object";
				Out << YAML::BeginMap;
				int ObjectType = -1;
				if (const FPickupItem* Object = std::get_if<FPickupItem>(&Data.Object)) {
					ObjectType = static_cast<int>(Object->Type);
					LK_FATAL_TAG("Serializer", "PickupItem ObjectType={}", ObjectType);

					EItemPayload PayloadKind = EItemPayload::None;
					if (std::holds_alternative<FConsumablePayload>(Object->Payload)) {
						PayloadKind = EItemPayload::Consumable;
					} else if (std::holds_alternative<FAmmoPayload>(Object->Payload)) {
						PayloadKind = EItemPayload::Ammo;
					}
					Out << YAML::Key << "PayloadKind" << YAML::Value << std::to_underlying(PayloadKind);
					if (const FConsumablePayload* P = std::get_if<FConsumablePayload>(&Object->Payload)) {
						Out << YAML::Key << "Consumable";
						Out << YAML::BeginMap;
						Out << YAML::Key << "Kind" << YAML::Value << std::to_underlying(P->Kind);
						Out << YAML::Key << "Amount" << YAML::Value << P->Amount;
						Out << YAML::EndMap;
					} else if (const FAmmoPayload* P = std::get_if<FAmmoPayload>(&Object->Payload)) {
						Out << YAML::Key << "Ammo";
						Out << YAML::BeginMap;
						Out << YAML::Key << "Weapon" << YAML::Value << std::to_underlying(P->Weapon);
						Out << YAML::Key << "Count" << YAML::Value << P->Count;
						Out << YAML::EndMap;
					}
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
			case EInteraction::Heal:
			{
				const auto& Data = std::get<FHealInteraction>(IC.Data);
				Out << YAML::Key << "Amount" << YAML::Value << Data.Amount;
				Out << YAML::Key << "ConsumeOnUse" << YAML::Value << Data.bConsumeOnUse;
				break;
			}
			case EInteraction::Killzone:
			{
				break;
			}
			case EInteraction::Jumppad:
			{
				const auto& Data = std::get<FJumppadInteraction>(IC.Data);
				Out << YAML::Key << "Impulse" << YAML::Value << Data.Impulse;
				Out << YAML::Key << "PreserveHorizontalVelocity" << YAML::Value << Data.bPreserveHorizontalVelocity;
				break;
			}
			case EInteraction::Climbable:
			{
				const auto& Data = std::get<FClimbableInteraction>(IC.Data);
				Out << YAML::Key << "ClimbSpeed" << YAML::Value << Data.ClimbSpeed;
				break;
			}
			case EInteraction::Checkpoint:
			{
				const auto& Data = std::get<FCheckpointInteraction>(IC.Data);
				Out << YAML::Key << "CheckpointID" << YAML::Value << Data.CheckpointID;
				break;
			}
			default:
				LK_ERROR_TAG("Serializer", "Interaction {} not supported", Enum::ToString(IC.Type));
				break;
		}

		Out << YAML::EndMap;
	}

	template<>
	inline void Serialize(const FHealthComponent& HC, YAML::Emitter& Out)
	{
		Out << YAML::Key << "HealthComponent";
		Out << YAML::BeginMap;
		Out << YAML::Key << "MaxHealth" << YAML::Value << HC.MaxHealth;
		Out << YAML::Key << "Damageable" << YAML::Value << HC.bDamageable;
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
			case EInteraction::Damage:
			{
				FDamageInteraction Data;
				DeserializeProperty("Damage", Data.Damage, 0.0f, Node);
				IC.Data = Data;
				LK_DEBUG_TAG("Deserializer", "FDamageInteraction::Damage: {}", Data.Damage);
				break;
			}
			case EInteraction::Pickup:
			{
				FPickupInteraction Data;
				DeserializeProperty("PickupKind", Data.Kind, EPickupKind::Item, Node);
				DeserializeProperty("ExpireWhenPickedUp", Data.bExpireWhenPickedUp, false, Node);

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
						FPickupItem Object = {.Type = static_cast<EItemType>(ObjectTypeValue)};

						const YAML::Node& ObjectNode = Node["Object"];
						if (ObjectNode.IsDefined() && ObjectNode["PayloadKind"]) {
							const auto PayloadKind = static_cast<EItemPayload>(ObjectNode["PayloadKind"].as<std::underlying_type_t<EItemPayload>>());
							if ((PayloadKind == EItemPayload::Consumable) && ObjectNode["Consumable"]) {
								const YAML::Node& Pn = ObjectNode["Consumable"];
								FConsumablePayload P;
								DeserializeProperty("Kind", P.Kind, EConsumableKind::Health, Pn);
								DeserializeProperty("Amount", P.Amount, 25.0f, Pn);
								Object.Payload = P;
							} else if ((PayloadKind == EItemPayload::Ammo) && ObjectNode["Ammo"]) {
								const YAML::Node& Pn = ObjectNode["Ammo"];
								FAmmoPayload P;
								DeserializeProperty("Weapon", P.Weapon, EWeaponType::Rifle, Pn);
								std::uint16_t Count = 30;
								DeserializeProperty("Count", Count, std::uint16_t(30), Pn);
								P.Count = Count;
								Object.Payload = P;
							}
						}

						Data.Object = Object;
					} else if (Data.Kind == EPickupKind::Weapon) {
						FPickupWeapon Object = {
							.Type = static_cast<EWeaponType>(ObjectTypeValue)};

						const YAML::Node& ObjectNode = Node["Object"];
						LK_ASSERT(ObjectNode.IsDefined(), "Object node not defined");
						if (Object.Type == EWeaponType::Rifle) {
							const YAML::Node& SpecNode = ObjectNode["Specification"];
							FRifleSpecification Spec;
							DeserializeProperty("MagazineSize", Spec.MagazineSize, 30, SpecNode);
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
			case EInteraction::Heal:
			{
				FHealInteraction Data;
				DeserializeProperty("Amount", Data.Amount, 25.0f, Node);
				DeserializeProperty("ConsumeOnUse", Data.bConsumeOnUse, true, Node);
				IC.Data = Data;
				break;
			}
			case EInteraction::Killzone:
			{
				IC.Data = FKillzoneInteraction{};
				break;
			}
			case EInteraction::Jumppad:
			{
				FJumppadInteraction Data;
				DeserializeProperty("Impulse", Data.Impulse, glm::vec2(0.0f, 6.0f), Node);
				DeserializeProperty("PreserveHorizontalVelocity", Data.bPreserveHorizontalVelocity, true, Node);
				IC.Data = Data;
				break;
			}
			case EInteraction::Climbable:
			{
				FClimbableInteraction Data;
				DeserializeProperty("ClimbSpeed", Data.ClimbSpeed, 1.0f, Node);
				IC.Data = Data;
				break;
			}
			case EInteraction::Checkpoint:
			{
				FCheckpointInteraction Data;
				DeserializeProperty("CheckpointID", Data.CheckpointID, std::string{}, Node);
				IC.Data = Data;
				break;
			}
			default:
				LK_ERROR_TAG("Deserializer", "Interaction {} not supported", Enum::ToString(IC.Type));
				break;
		}
	}

	template<>
	inline void Deserialize(FHealthComponent& HC, const YAML::Node& Node)
	{
		HC.MaxHealth = Node["MaxHealth"].as<decltype(HC.MaxHealth)>();
		HC.Health = HC.MaxHealth;
		HC.bDamageable = Node["Damageable"].as<bool>();
	}

	template<>
	inline void Deserialize(FBodySpecification& BodySpec, const YAML::Node& Node)
	{
		LK_ASSERT(Node["Type"] && Node["Shape"]);
		BodySpec.Type = static_cast<EBodyType>(Node["Type"].as<int>());

		DeserializeProperty("GravityScale", BodySpec.GravityScale, 1.0f, Node);

		const YAML::Node ShapeNode = Node["Shape"];
		LK_VERIFY(ShapeNode["ShapeType"], "ShapeType missing in yaml");
		const EShape ShapeType = static_cast<EShape>(ShapeNode["ShapeType"].as<int>());
		switch (ShapeType) {
			case EShape::Polygon:
			{
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
			case EShape::Line:
			{
				LK_MARK_NOT_IMPLEMENTED();
				break;
			}
			case EShape::Capsule:
			{
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
			case EShape::Chain:
			{
				FChain Chain;
				if (ShapeNode["Loop"]) {
					Chain.bLoop = ShapeNode["Loop"].as<bool>();
				}
				if (ShapeNode["Friction"]) {
					Chain.Friction = ShapeNode["Friction"].as<float>();
				}
				if (ShapeNode["TextureHeight"]) {
					Chain.TextureHeight = ShapeNode["TextureHeight"].as<float>();
				}
				const YAML::Node Points = ShapeNode["Points"];
				if (Points && Points.IsSequence()) {
					Chain.Points.reserve(Points.size());
					for (const YAML::Node& PointNode : Points) {
						Chain.Points.push_back(PointNode.as<glm::vec2>());
					}
				}
				LK_DEBUG("Deserialize: Chain: Points={} Loop={}", Chain.Points.size(), Chain.bLoop);
				BodySpec.Shape.emplace<FChain>(Chain);
				break;
			}
			case EShape::None:
				break;
		}

		using PosType = decltype(BodySpec.Position);
		BodySpec.Position = Node["Position"].as<PosType>();

		using FlagsType = std::underlying_type_t<EBodyFlag>;
		BodySpec.Flags = Node["Flags"].as<FlagsType>();

		BodySpec.Density = Node["Mass"].as<float>(); /** @todo Density <-> Mass, equivalent in terms of body creation? */
		BodySpec.MotionLock = Node["MotionLock"].as<std::underlying_type_t<EMotionLock>>();

		DeserializeProperty("Sensor", BodySpec.bSensor, false, Node);
	}

}
