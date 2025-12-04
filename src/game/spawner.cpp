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
		ActorSpec.Name = Name;
		ActorSpec.Texture = ETexture::White;
		ActorSpec.Color = Color;

		FBodySpecification BodySpec;
		BodySpec.Type = EBodyType::Static;
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
		ActorSpec.Name = Name;
		ActorSpec.Texture = Texture;
		ActorSpec.Color = Color;

		FBodySpecification& BodySpec = const_cast<FBodySpecification&>(InBodySpec);
		FPolygon Polygon = {
			.Size = Size,
		};
		BodySpec.Shape.emplace<FPolygon>(Polygon);

		LK_INFO_TAG("Spawner", "Create: {}  Texture={} Color={}", Name, Enum::ToString(Texture), Color);
		std::shared_ptr<CActor> Actor = Scene->Create<CActor>(ActorSpec, BodySpec);
		return Actor;
	}

	std::shared_ptr<CActor> CSpawner::CreateSpawnpoint(std::string_view Name, const glm::vec2& Pos)
	{
		LK_VERIFY(!Name.empty(), "Name cannot be empty");
		CGameInstance* GameInstance = CGameInstance::Get();
		LK_VERIFY(GameInstance);
		std::shared_ptr<CScene> Scene = GameInstance->GetScene();

		FActorSpecification ActorSpec;
		ActorSpec.Name = Name;
		ActorSpec.Texture = ETexture::White;
		ActorSpec.Color = FColor::Transparent;
		ActorSpec.Pos = glm::vec3(Pos, 0.0f);
		ActorSpec.OutlineEnabled = true;
		ActorSpec.OutlineColor = FColor::Magenta;
		ActorSpec.OutlineThickness = 6.0f;

		std::shared_ptr<CActor> Spawnpoint = Scene->Create<CActor>(ActorSpec);
		Spawnpoint->SetSize({ 0.25f, 0.25f });

		return Spawnpoint;
	}

}
