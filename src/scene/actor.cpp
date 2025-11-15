#include "actor.h"

#include "core/log.h"

namespace platformer2d {

	CActor::CActor(const FActorSpecification& Spec)
		: Handle(GenerateHandle())
		, Name("")
		, Texture(Spec.Texture)
		, Color(Spec.Color)
	{
	}

	CActor::CActor(const LUUID InHandle, const FBodySpecification& BodySpec, ETexture InTexture, const glm::vec4& InColor)
		: Handle(InHandle)
		, Name(BodySpec.Name)
		, Texture(InTexture)
		, Color(InColor)
	{
		LK_TRACE_TAG("Actor", "Create: {} ({})", (!Name.empty() ? Name : "NULL"), Handle);
		Body = std::make_unique<CBody>(BodySpec);
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

	CActor::CActor(const FBodySpecification& BodySpec, ETexture InTexture, const glm::vec4& InColor)
		: CActor(GenerateHandle(), BodySpec, InTexture, InColor)
	{
	}

	CActor::~CActor()
	{
		LK_DEBUG_TAG("Actor", "Release: {} ({})", Name, Handle);
	}

	void CActor::Tick(const float DeltaTime)
	{
		if (bTickEnabled)
		{
			Body->Tick(DeltaTime);

			const glm::vec2 BodyPos = Body->GetPosition();
			TransformComp.Translation.x = BodyPos.x;
			TransformComp.Translation.y = BodyPos.y;
			TransformComp.SetRotation2D(Body->GetRotation());
		}
	}

	glm::vec2 CActor::GetSize() const
	{
		return Body ? Body->GetSize() : glm::vec2(0.0f, 0.0f);
	}

	glm::vec2 CActor::GetPosition() const
	{
		return glm::vec2(TransformComp.Translation.x, TransformComp.Translation.y);
	}

	void CActor::SetPosition(const float X, const float Y)
	{
		SetPosition({ X, Y });
	}

	void CActor::SetPosition(const glm::vec2& NewPos)
	{
		TransformComp.Translation.x = NewPos.x;
		TransformComp.Translation.y = NewPos.y;
		Body->SetPosition(NewPos);
	}

	float CActor::GetRotation() const
	{
		return TransformComp.GetRotation2D();
	}

	void CActor::SetRotation(const float AngleRad)
	{
		Body->SetRotation(AngleRad);
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
		LK_TRACE_TAG("Actor", "[{}] Tick {}", Name, Enabled ? "enabled" : "disabled");
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

	bool CActor::Serialize(YAML::Emitter& Out) const
	{
		LK_TRACE_TAG("Actor", "Serialize: {} (Handle: {})", Name, Handle);
		Out << YAML::BeginMap; /* Actor */
		Out << YAML::Key << "ID";
		Out << YAML::Value << Handle;
		Out << YAML::Key << "Name";
		Out << YAML::Value << Name;

		Out << YAML::Key << "Texture";
		Out << YAML::Value << std::to_underlying(Texture);
		Out << YAML::Key << "Color";
		Out << YAML::Value << Color;

		/* TransformComponent */
		const FTransformComponent& TC = GetTransformComponent();
		Out << YAML::Key << "TransformComponent";
		Out << YAML::BeginMap;
		Out << YAML::Key << "Position" << YAML::Value << TC.Translation;
		Out << YAML::Key << "Rotation" << YAML::Value << TC.GetRotation2D();
		Out << YAML::Key << "Scale" << YAML::Value << TC.Scale;
		Out << YAML::EndMap;
		/* ~TransformComponent */

		if (Body)
		{
			Body->Serialize(Out);
		}

		Out << YAML::Key << "Deletable";
		Out << YAML::Value << bDeletable;

		Out << YAML::EndMap; /* ~Actor */

		return true;
	}

	LUUID CActor::GenerateHandle()
	{
		Instances++;
		return Instances;
	}

}