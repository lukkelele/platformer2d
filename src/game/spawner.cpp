#include "spawner.h"

#include "game/instance.h"
#include "scene/actor.h"
#include "scene/scene.h"

namespace platformer2d {

	std::shared_ptr<CActor> CSpawner::CreateStaticPolygon(std::string_view Name, const glm::vec2& Pos,
		const glm::vec2& Size, const glm::vec4& Color, const ETexture Texture)
	{
		std::string ActorName(Name);
		if (ActorName.empty()) {
			ActorName = Format("StaticPolygon{}", static_cast<std::uint16_t>(Math::Randomize(0, std::numeric_limits<std::uint16_t>::max())));
		}
		LK_VERIFY(CGameInstance::IsValid());
		std::shared_ptr<CScene> Scene = CGameInstance::Get().GetScene();

		FActorSpecification ActorSpec;
		ActorSpec.Name = ActorName;
		ActorSpec.Texture = Texture;
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

	std::shared_ptr<CActor> CSpawner::CreateStaticPolygon(std::string_view Name, const glm::vec2& Pos,
		const glm::vec2& Size, const FBodySpecification& BodySpec, const glm::vec4& Color, const ETexture Texture)
	{
		std::string ActorName(Name);
		if (ActorName.empty()) {
			ActorName = Format("StaticPolygon{}", static_cast<std::uint16_t>(Math::Randomize(0, std::numeric_limits<std::uint16_t>::max())));
		}
		LK_VERIFY(CGameInstance::IsValid());
		std::shared_ptr<CScene> Scene = CGameInstance::Get().GetScene();

		FActorSpecification ActorSpec;
		ActorSpec.Name = ActorName;
		ActorSpec.Texture = Texture;
		ActorSpec.Color = Color;

		FPolygon Polygon = {
			.Size = Size,
		};
		auto& BodySpecRef = *const_cast<FBodySpecification*>(&BodySpec);
		BodySpecRef.Position = Pos;
		BodySpecRef.Shape.emplace<FPolygon>(Polygon);

		LK_INFO_TAG("Spawner", "Create: {}", Name);
		std::shared_ptr<CActor> Actor = Scene->Create<CActor>(ActorSpec, BodySpec);
		return Actor;
	}

	std::shared_ptr<CActor> CSpawner::CreatePolygon(std::string_view Name, const FBodySpecification& InBodySpec,
		const glm::vec2& Size, const glm::vec4& Color, const ETexture Texture)
	{
		std::string ActorName(Name);
		if (ActorName.empty()) {
			ActorName = Format("Polygon{}", static_cast<std::uint16_t>(Math::Randomize(0, std::numeric_limits<std::uint16_t>::max())));
		}
		LK_VERIFY(CGameInstance::IsValid());
		std::shared_ptr<CScene> Scene = CGameInstance::Get().GetScene();

		FActorSpecification ActorSpec;
		ActorSpec.Name = ActorName;
		ActorSpec.Texture = Texture;
		ActorSpec.Color = Color;

		FBodySpecification& BodySpec = const_cast<FBodySpecification&>(InBodySpec);
		FPolygon Polygon = {
			.Size = Size,
		};
		BodySpec.Shape.emplace<FPolygon>(Polygon);

		LK_INFO_TAG("Spawner", "Create: {} (Texture={} Color={})", Name, Enum::ToString(Texture), Color);
		std::shared_ptr<CActor> Actor = Scene->Create<CActor>(ActorSpec, BodySpec);
		return Actor;
	}

	std::shared_ptr<CActor> CSpawner::CreateChain(std::string_view Name, std::span<const glm::vec2> Points,
		const bool Loop, const bool BlockBothSides, const glm::vec4& Color)
	{
		LK_ASSERT(Points.size() >= 4, "Chain requires at least 4 points");
		std::string ActorName(Name);
		if (ActorName.empty()) {
			ActorName = Format("Chain{}", static_cast<std::uint16_t>(Math::Randomize(0, std::numeric_limits<std::uint16_t>::max())));
		}
		LK_VERIFY(CGameInstance::IsValid());
		std::shared_ptr<CScene> Scene = CGameInstance::Get().GetScene();

		FActorSpecification ActorSpec;
		ActorSpec.Name = ActorName;
		ActorSpec.Texture = ETexture::White;
		ActorSpec.Color = Color;
		ActorSpec.OutlineEnabled = false;
		ActorSpec.Flags = EActorFlag::EActorFlag_Terrain;

		FBodySpecification BodySpec;
		BodySpec.Type = EBodyType::Static;
		BodySpec.Position = {0.0f, 0.0f};
		BodySpec.Flags = EBodyFlag_PreSolveEvents;

		FChain Chain;
		Chain.Points.assign(Points.begin(), Points.end());
		Chain.bLoop = Loop;
		Chain.bBlockBothSides = BlockBothSides;
		BodySpec.Shape.emplace<FChain>(Chain);

		LK_INFO_TAG("Spawner", "Create chain: {} ({} points, loop={} both-sides={})",
			ActorName, Points.size(), Loop, BlockBothSides);
		std::shared_ptr<CActor> Actor = Scene->Create<CActor>(ActorSpec, BodySpec);
		return Actor;
	}

	std::shared_ptr<CActor> CSpawner::CreateSpawnpoint(std::string_view Name, const glm::vec2& Pos)
	{
		LK_VERIFY(!Name.empty(), "Name cannot be empty");
		LK_VERIFY(CGameInstance::IsValid());
		std::shared_ptr<CScene> Scene = CGameInstance::Get().GetScene();

		FActorSpecification ActorSpec;
		ActorSpec.Name = Name;
		ActorSpec.Texture = ETexture::White;
		ActorSpec.Color = FColor::Transparent;
		ActorSpec.Pos = glm::vec3(Pos, 0.0f);
		ActorSpec.OutlineEnabled = true;
		ActorSpec.OutlineColor = FColor::Magenta;
		ActorSpec.OutlineThickness = 6.0f;

		std::shared_ptr<CActor> Spawnpoint = Scene->Create<CActor>(ActorSpec);
		Spawnpoint->SetSize({0.25f, 0.25f});

		return Spawnpoint;
	}

}
