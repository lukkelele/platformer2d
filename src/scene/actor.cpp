#include "actor.h"

#include "core/log.h"
#include "serialization/serialization.h"

namespace platformer2d {

	CActor::CActor(const FActorSpecification& Spec)
		: Handle(LUUID())
		, Name("")
		, Texture(Spec.Texture)
		, Color(Spec.Color)
	{
	}

	CActor::CActor(const FActorSpecification& InSpec, const FBodySpecification& BodySpec)
		: Handle(InSpec.Handle)
		, Name(BodySpec.Name)
		, Texture(InSpec.Texture)
		, Color(InSpec.Color)
		, Outline(InSpec.OutlineEnabled, InSpec.OutlineThickness, InSpec.OutlineColor)
	{
		LK_TRACE_TAG("Actor", "Create: {} ({})", (!Name.empty() ? Name : "NULL"), Handle);
		Body = std::make_unique<CBody>(BodySpec, this);
		const glm::vec2 BodyPos = Body->GetPosition();
		TransformComp.Translation.x = BodyPos.x;
		TransformComp.Translation.y = BodyPos.y;
		TransformComp.SetRotation2D(Body->GetRotation());

		if (const FPolygon* Polygon = std::get_if<FPolygon>(&BodySpec.Shape); Polygon != nullptr)
		{
			LK_TRACE_TAG("Actor", "[{}] Scaling polygon -> {}", Handle, Polygon->Size);
			TransformComp.SetScale(Polygon->Size);
		}
	}

	CActor::~CActor()
	{
		LK_DEBUG_TAG("Actor", "Release: {} ({})", Name, Handle);
	}

	void CActor::Tick(const float DeltaTime)
	{
		if (!bTickEnabled)
		{
			return;
		}

		if (Body)
		{
			Body->Tick(DeltaTime);

			const glm::vec2 BodyPos = Body->GetPosition();
			TransformComp.Translation.x = BodyPos.x;
			TransformComp.Translation.y = BodyPos.y;
			TransformComp.SetRotation2D(Body->GetRotation());
		}

		if (FEffectComponent* EC = TryGetComponent<FEffectComponent>(); EC != nullptr)
		{
			UpdateEffectComponent(*EC);
		}
	}

	glm::vec2 CActor::GetSize() const
	{
		return Body ? Body->GetSize() : glm::vec2(0.0f, 0.0f);
	}

	glm::vec3 CActor::GetPosition() const
	{
		return TransformComp.Translation;
	}

	void CActor::SetPosition(const float X, const float Y)
	{
		SetPosition({ X, Y });
	}

	void CActor::SetPosition(const glm::vec2& NewPos)
	{
		TransformComp.Translation.x = NewPos.x;
		TransformComp.Translation.y = NewPos.y;
		Body->SetPosition({ TransformComp.Translation.x, TransformComp.Translation.y });
	}

	void CActor::SetPosition(const glm::vec3& NewPos)
	{
		TransformComp.Translation = NewPos;
		Body->SetPosition({ TransformComp.Translation.x, TransformComp.Translation.y });
	}

	float CActor::GetRotation() const
	{
		return TransformComp.GetRotation2D();
	}

	void CActor::SetRotation(const float AngleRad)
	{
		if (Body)
		{
			Body->SetRotation(AngleRad);
		}
		TransformComp.SetRotation2D(AngleRad);
	}

	bool CActor::IsMoving() const
	{
		if (!Body)
		{
			return false;
		}

		return (Body->GetLinearVelocity().x > CBody::LINEAR_VELOCITY_X_EPSILON);
	}

	void CActor::SetTickEnabled(const bool Enabled)
	{
		bTickEnabled = Enabled;
	}

	void CActor::SetDeletable(const bool Deletable)
	{
		bDeletable = Deletable;
	}

	void CActor::SetColor(const glm::vec4& InColor)
	{
		Color = InColor;
	}

	void CActor::SetName(std::string_view InName)
	{
		if (Name != InName)
		{
			LK_DEBUG_TAG("Actor", "Changed name to \"{}\" from \"{}\"", InName, Name);
			Name = InName;
		}
	}

	void CActor::SetTexture(const ETexture InTexture)
	{
		LK_DEBUG_TAG("Actor", "Set texture: {}", Enum::ToString(InTexture));
		if (Texture == InTexture)
		{
			LK_TRACE_TAG("Actor", "Same texture, leave unchanged");
			return;
		}

		Texture = InTexture;
	}

	void CActor::SetOutlineEnabled(const bool Enabled)
	{
		Outline.bEnabled = Enabled;
	}

	void CActor::SetOutlineThickness(const float InThickness)
	{
		Outline.Thickness = InThickness;
	}

	void CActor::SetOutlineColor(const glm::vec4& InColor)
	{
		Outline.Color = InColor;
	}

	bool CActor::Serialize(YAML::Emitter& Out) const
	{
		LK_TRACE_TAG("Actor", "Serialize: {} (Handle: {})", Name, Handle);
		Out << YAML::BeginMap; /* Actor */
		Out << YAML::Key << "ID" << YAML::Value << Handle;
		Out << YAML::Key << "Type" << YAML::Value << std::to_underlying(GetType());
		Out << YAML::Key << "Name" << YAML::Value << Name;
		Out << YAML::Key << "Texture" << YAML::Value << std::to_underlying(Texture);
		Out << YAML::Key << "Color" << YAML::Value << Color;

		Out << YAML::Value << "Outline";
		Out << YAML::BeginMap;
		Out << YAML::Key << "Enabled" << YAML::Value << Outline.bEnabled;
		Out << YAML::Key << "Thickness" << YAML::Value << Outline.Thickness;
		Out << YAML::Key << "Color" << YAML::Value << Outline.Color;
		Out << YAML::EndMap;

		/* TransformComponent */
		const FTransformComponent& TC = GetTransformComponent();
		Serialization::Serialize(TC, Out);
		/* ~TransformComponent */

		/* EffectComponent */
		if (HasComponent<FEffectComponent>())
		{
			const auto& EC = GetComponent<FEffectComponent>();
			Serialization::Serialize(EC, Out);
		}
		/* ~EffectComponent */

		/* InteractionComponent */
		if (HasComponent<FInteractionComponent>())
		{
			const auto& IC = GetComponent<FInteractionComponent>();
			Serialization::Serialize(IC, Out);
		}
		/* ~InteractionComponent */

		if (Body)
		{
			Body->Serialize(Out);
		}

		Out << YAML::Key << "Deletable";
		Out << YAML::Value << bDeletable;

		Out << YAML::EndMap; /* ~Actor */

		return true;
	}

	void CActor::UpdateEffectComponent(FEffectComponent& EC)
	{
		for (auto& Effect : EC.Effects)
		{
			switch (Effect.Type)
			{
				case EEffectType::Rotate:
				{
					if (Body)
					{
						const FRotateEffect& Rotate = std::get<FRotateEffect>(Effect.Data);
						Body->SetAngularVelocity(glm::radians(Rotate.AngularSpeedDegPerSecond));
					}
					break;
				}

				default:
					break;
			}
		}
	}

}