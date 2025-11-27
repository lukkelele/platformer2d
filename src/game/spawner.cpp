#include "spawner.h"

#include "game/instance.h"
#include "scene/scene.h"

namespace platformer2d {

	std::shared_ptr<CActor> CSpawner::CreateStaticPolygon(std::string_view Name, const glm::vec2& Pos,
														  const glm::vec2& Size, const glm::vec4& Color)
	{
		LK_VERIFY(!Name.empty(), "Name cannot be empty");
		CGameInstance* GameInstance = CGameInstance::Get();
		LK_VERIFY(GameInstance);
		std::shared_ptr<CScene> Scene = GameInstance->GetScene();

		FBodySpecification Spec;
		Spec.Type = EBodyType::Static;
		Spec.Name = Name;
		Spec.Position = Pos;
		Spec.Flags = EBodyFlag_PreSolveEvents;

		FPolygon Polygon = {
			.Size = Size,
		};
		Spec.Shape.emplace<FPolygon>(Polygon);

		LK_INFO_TAG("Spawner", "Create: {}", Name);
		std::shared_ptr<CActor> Actor = Scene->Create<CActor>(Spec, ETexture::White, Color);
		return Actor;
	}

	std::shared_ptr<CActor> CSpawner::CreatePolygon(std::string_view Name, const FBodySpecification& BodySpec,
													const glm::vec2& Size, const glm::vec4& Color, const ETexture Texture)
	{
		LK_VERIFY(!Name.empty(), "Name cannot be empty");
		CGameInstance* GameInstance = CGameInstance::Get();
		LK_VERIFY(GameInstance);
		std::shared_ptr<CScene> Scene = GameInstance->GetScene();

		FBodySpecification& Spec = const_cast<FBodySpecification&>(BodySpec);
		Spec.Name = Name;
		FPolygon Polygon = {
			.Size = Size,
		};
		Spec.Shape.emplace<FPolygon>(Polygon);

		LK_INFO_TAG("Spawner", "Create: {}  Texture={} Color={}", Name, Enum::ToString(Texture), Color);
		std::shared_ptr<CActor> Actor = Scene->Create<CActor>(Spec, Texture, Color);
		return Actor;
	}

}
