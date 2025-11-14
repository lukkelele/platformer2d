#include "spawner.h"

namespace platformer2d {

	std::shared_ptr<CActor> CSpawner::CreateStaticPolygon(std::string_view Name, const glm::vec2& Pos,
														  const glm::vec2& Size, const glm::vec4& Color)
	{
		LK_VERIFY(!Name.empty(), "Name cannot be empty");
		FBodySpecification Spec;
		Spec.Type = EBodyType::Static;
		Spec.Name = Name;
		Spec.Position = Pos;
		Spec.Flags = EBodyFlag_PreSolveEvents;

		FPolygon Polygon = {
			.Size = Size,
		};
		Spec.Shape.emplace<FPolygon>(Polygon);

		std::shared_ptr<CActor> Actor = CActor::Create<CActor>(Spec, ETexture::White, Color);

		return Actor;
	}

}
