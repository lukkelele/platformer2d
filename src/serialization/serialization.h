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
		if (Node.IsDefined()) \
		{ \
			if (auto NodeRef = Node[#PropertyName]) \
			{ \
				try \
				{ \
					Destination = NodeRef.as<decltype(DefaultValue)>(); \
				} \
				catch (const std::exception& Exception) \
				{ \
					LK_ERROR_TAG("Deserializer", "Failed to get \"{}\": {}", #PropertyName, Exception.what()); \
					Destination = DefaultValue; \
				} \
			} \
		} \
		else \
		{ \
			LK_ERROR_TAG("Deserializer", "Default value used for: {}", #PropertyName); \
			Destination = DefaultValue; \
		}

	template<typename T>
	static void Serialize(const T& Target, YAML::Emitter& Out)
	{
		LK_UNUSED(Target, Out);
	}

	template<>
	void Serialize(const FTransformComponent& TC, YAML::Emitter& Out)
	{
		Out << YAML::Key << "TransformComponent";
		Out << YAML::BeginMap;
		Out << YAML::Key << "Position" << YAML::Value << TC.Translation;
		Out << YAML::Key << "Rotation" << YAML::Value << TC.GetRotation2D();
		Out << YAML::Key << "Scale" << YAML::Value << TC.Scale;
		Out << YAML::EndMap;
	}

	template<>
	void Serialize(const FEffectComponent& EC, YAML::Emitter& Out)
	{
		if (EC.Effects.empty())
		{
			LK_DEBUG_TAG("Serializer", "EffectComponent has no effects");
			return;
		}

		Out << YAML::Key << "EffectComponent";
		Out << YAML::Value << YAML::BeginSeq;
		for (std::size_t Index = 0; Index < EC.Effects.size(); Index++)
		{
			const FEffectInstance& Instance = EC.Effects[Index];
			YAML::Node EffectNode;

			Out << YAML::BeginMap;
			Out << YAML::Key << "Type" << YAML::Value << Enum::ToString(Instance.Type);

			switch (Instance.Type)
			{
				case EEffectType::Rotate:
				{
					const FRotateEffect& Rotate = std::get<FRotateEffect>(Instance.Data);
					Out << YAML::Key << "AngularSpeedDegPerSecond";
					Out << YAML::Value << Rotate.AngularSpeedDegPerSecond;
					break;
				}

				default: break;
			}

			Out << YAML::EndMap;
		}

		Out << YAML::EndSeq;
	}

	template<>
	void Serialize(const FInteractionComponent& IC, YAML::Emitter& Out)
	{
		Out << YAML::Key << "InteractionComponent";
		Out << YAML::BeginMap;
		Out << YAML::Key << "Type" << YAML::Value << std::to_underlying(IC.GetType());
		Out << YAML::EndMap;
	}

	template<typename T>
	static void Deserialize(T& Target, const YAML::Node& Node)
	{
		LK_UNUSED(Target, Node);
	}

	template<>
	void Deserialize(FTransformComponent& TC, const YAML::Node& Node)
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
		for (std::size_t Idx = 0; Idx < NodeSeq.size(); Idx++)
		{
			const YAML::Node& EffectNode = NodeSeq[Idx];
			if (!EffectNode)
			{
				continue;
			}

			FEffectInstance Instance;
			Instance.Type = EEffectType::None;
			Instance.Data = std::monostate{};

			const std::string TypeString = EffectNode["Type"].as<std::string>();
			const EEffectType Type = Enum::FromString(TypeString);
			LK_TRACE("Type={} TypeString={}", std::to_underlying(Type), TypeString);

			if (TypeString == "Rotate")
			{
				Instance.Type = EEffectType::Rotate;
				FRotateEffect Rotate;
				Rotate.AngularSpeedDegPerSecond = EffectNode["AngularSpeedDegPerSecond"].as<float>();
				Instance.Data = Rotate;
			}

			if (Instance.Type != EEffectType::None)
			{
				LK_TRACE("Push effect: {}", TypeString);
				EC.Effects.push_back(Instance);
			}
		}
	}

	template<>
	void Deserialize(FInteractionComponent& IC, const YAML::Node& Node)
	{
		IC.Type = static_cast<EInteraction>(Node["Type"].as<std::underlying_type_t<EInteraction>>());
	}

	template<>
	void Deserialize(FBodySpecification& BodySpec, const YAML::Node& Node)
	{
		LK_ASSERT(Node["Type"] && Node["Shape"]);
		BodySpec.Type = static_cast<EBodyType>(Node["Type"].as<int>());

		LK_DESERIALIZE_PROPERTY(GravityScale, BodySpec.GravityScale, 1.0f, Node);

		const YAML::Node ShapeNode = Node["Shape"];
		LK_VERIFY(ShapeNode["ShapeType"], "ShapeType missing in yaml");
		const EShape ShapeType = static_cast<EShape>(ShapeNode["ShapeType"].as<int>());
		switch (ShapeType)
		{
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
