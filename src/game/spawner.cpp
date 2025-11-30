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

		FActorSpecification ActorSpec;
		ActorSpec.Texture = ETexture::White;
		ActorSpec.Color = Color;

		FBodySpecification BodySpec;
		BodySpec.Type = EBodyType::Static;
		BodySpec.Name = Name;
		BodySpec.Position = Pos;
		BodySpec.Flags = EBodyFlag_PreSolveEvents;

		FPolygon Polygon = {
			.Size = Size,
		};
		BodySpec.Shape.emplace<FPolygon>(Polygon);

		LK_INFO_TAG("Spawner", "Create: {}", Name);
		std::shared_ptr<CActor> Actor = Scene->Create<CActor>(ActorSpec, BodySpec);
		return Actor;
	}

	std::shared_ptr<CActor> CSpawner::CreatePolygon(std::string_view Name, const FBodySpecification& InBodySpec,
													const glm::vec2& Size, const glm::vec4& Color, const ETexture Texture)
	{
		LK_VERIFY(!Name.empty(), "Name cannot be empty");
		CGameInstance* GameInstance = CGameInstance::Get();
		LK_VERIFY(GameInstance);
		std::shared_ptr<CScene> Scene = GameInstance->GetScene();

		FActorSpecification ActorSpec;
		ActorSpec.Texture = Texture;
		ActorSpec.Color = Color;

		FBodySpecification& BodySpec = const_cast<FBodySpecification&>(InBodySpec);
		BodySpec.Name = Name;
		FPolygon Polygon = {
			.Size = Size,
		};
		BodySpec.Shape.emplace<FPolygon>(Polygon);

		LK_INFO_TAG("Spawner", "Create: {}  Texture={} Color={}", Name, Enum::ToString(Texture), Color);
		std::shared_ptr<CActor> Actor = Scene->Create<CActor>(ActorSpec, BodySpec);
		return Actor;
	}

}
