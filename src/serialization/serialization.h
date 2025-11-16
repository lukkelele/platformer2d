#pragma once

#include "core/core.h"
#include "physics/body.h"
#include "scene/components.h"
#include "yaml.h"

namespace platformer2d::Serialization {

	template<typename T>
	static void Serialize(const T& Target, YAML::Emitter& Out)
	{
		LK_UNUSED(Target, Out);
	}

	template<>
	static void Serialize(const FTransformComponent& TC, YAML::Emitter& Out)
	{
		Out << YAML::Key << "TransformComponent";
		Out << YAML::BeginMap;
		Out << YAML::Key << "Position" << YAML::Value << TC.Translation;
		Out << YAML::Key << "Rotation" << YAML::Value << TC.GetRotation2D();
		Out << YAML::Key << "Scale" << YAML::Value << TC.Scale;
		Out << YAML::EndMap;
	}

	template<>
	static void Serialize(const FEffectComponent& EC, YAML::Emitter& Out)
	{
		if (EC.Effects.empty())
		{
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

	template<typename T>
	static void Deserialize(T& Target, const YAML::Node& Node)
	{
		LK_UNUSED(Target, Node);
	}

	template<>
	static void Deserialize(FTransformComponent& TC, const YAML::Node& Node)
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
	static void Deserialize(FEffectComponent& EC, const YAML::Node& NodeSeq)
	{
		LK_ASSERT(NodeSeq && NodeSeq.IsSequence());
		FEffectComponent Component;
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

			if (TypeString == "Rotate")
			{
				Instance.Type = EEffectType::Rotate;
				FRotateEffect Rotate;
				Rotate.AngularSpeedDegPerSecond = EffectNode["AngularSpeedDegPerSecond"].as<float>();
				Instance.Data = Rotate;
			}

			if (Instance.Type != EEffectType::None)
			{
				Component.Effects.push_back(Instance);
			}
		}
	}

	template<>
	static void Deserialize(FBodySpecification& BodySpec, const YAML::Node& Node)
	{
		LK_ASSERT(Node["Type"] && Node["Shape"]);
		BodySpec.Type = static_cast<EBodyType>(Node["Type"].as<int>());

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

		BodySpec.Flags = static_cast<EBodyFlag>(Node["Flags"].as<std::underlying_type_t<EBodyFlag>>());

		BodySpec.Density = Node["Mass"].as<float>(); /** @todo Density <-> Mass, equivalent in terms of body creation? */
		BodySpec.MotionLock = static_cast<EMotionLock>(Node["MotionLock"].as<std::underlying_type_t<EMotionLock>>());
	}

}
