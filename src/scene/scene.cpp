#include "scene.h"

#include <filesystem>
#include <fstream>
#include <istream>

#include "core/profiler.h"
#include "core/string.h"
#include "game/enemy.h"
#include "game/enemyspawner.h"
#include "game/instance.h"
#include "game/controller/patrolcontroller.h"
#include "renderer/renderer.h"
#include "renderer/sprite.h"
#include "serialization/serialization.h"

namespace platformer2d {

	CScene::CScene(std::string_view InName)
		: Name(InName)
	{
		LK_VERIFY(!Name.empty(), "Scene name is empty");
		Path = Format("{}/{}.{}", SCENES_DIR, Name, FILE_EXTENSION).c_str();
		LK_DEBUG_TAG("Scene", "Created: {} ({})", Name, StringUtils::GetPathRelativeToProject(Path));

		OnActorDeleted.Add([&](const LUUID Handle)
		{
			LK_DEBUG_TAG("Scene", "OnActorDeleted: {}", Handle);
			if (CGameInstance::IsValid()) {
				std::shared_ptr<CPlayer> Player = CGameInstance::Get().GetPlayer(0);
				Player->SetAwake(true);
			}
		});
	}

	CScene::~CScene()
	{
		LK_DEBUG_TAG("Scene", "Release: {} ({})", Name, ID);
		Actors.clear();
	}

	void CScene::Tick(const float DeltaTime)
	{
		LK_PROFILER_SCOPED();
		for (const auto& Actor : Actors) {
			Actor->Tick(DeltaTime);
		}
	}

	static void RenderChain(const CActor& Actor, const CBody& Body, const FChain& Chain)
	{
		const std::size_t Count = Chain.Points.size();
		if (Count < 2) {
			return;
		}

		const glm::vec2 Origin = Body.GetPosition();
		const glm::vec4& Color = Actor.GetColor();
		const ETexture Texture = Actor.GetTexture();
		const std::size_t Last = Chain.bLoop ? Count : (Count - 1);
		const bool Textured = (Texture != ETexture::White) && (Chain.TextureHeight > 0.0f);
		if (!Textured) {
			for (std::size_t Idx = 0; Idx < Last; Idx++) {
				const glm::vec2 P0 = Origin + Chain.Points[Idx];
				const glm::vec2 P1 = Origin + Chain.Points[(Idx + 1) % Count];
				CRenderer::DrawLine(P0, P1, Color, 4);
			}
			return;
		}

		const float SideSign = (Chain.TextureSide == EDirection::Up) ? 1.0f
			: (Chain.TextureSide == EDirection::Down)                ? -1.0f
																	 : 0.0f;
		const bool HasSide = (SideSign != 0.0f);
		const float HalfHeight = Chain.TextureHeight * 0.50f;
		const float TileWidth = (Chain.TextureTileWidth > 0.0f) ? Chain.TextureTileWidth : Chain.TextureHeight;
		const bool CanTile = Chain.bTextureTile && (TileWidth > 0.0f);

		for (std::size_t Idx = 0; Idx < Last; Idx++) {
			const glm::vec2 P0 = Origin + Chain.Points[Idx];
			const glm::vec2 P1 = Origin + Chain.Points[(Idx + 1) % Count];
			const glm::vec2 Edge = P1 - P0;
			const float Length = glm::length(Edge);
			if (Length <= 0.0f) {
				continue;
			}

			const glm::vec2 Dir = Edge / Length;
			const glm::vec2 Perp = {-Dir.y, Dir.x};
			const float AngleDeg = glm::degrees(std::atan2(Dir.y, Dir.x));
			const float PerpShift = (HasSide ? HalfHeight * SideSign : 0.0f) + Chain.TextureOffset;
			const glm::vec2 Shift = Perp * PerpShift;

			if (!CanTile) {
				const glm::vec2 Center = (P0 + P1) * 0.50f + Shift;
				CRenderer::DrawQuad(Center, glm::vec2(Length, Chain.TextureHeight), Texture, Color, AngleDeg);
				continue;
			}

			const std::size_t FullTiles = static_cast<std::size_t>(std::floor(Length / TileWidth));
			const float PartialLen = Length - (FullTiles * TileWidth);

			for (std::size_t TileIdx = 0; TileIdx < FullTiles; TileIdx++) {
				const float TileStart = TileIdx * TileWidth + (TileWidth * 0.50f);
				const glm::vec2 TileCenter = P0 + Dir * TileStart + Shift;
				CRenderer::DrawQuad(TileCenter, glm::vec2(TileWidth, Chain.TextureHeight), Texture, Color, AngleDeg);
			}

			if (PartialLen > 0.0f) {
				const float Frac = PartialLen / TileWidth;
				const float TileStart = FullTiles * TileWidth + (PartialLen * 0.50f);
				const glm::vec2 TileCenter = P0 + Dir * TileStart + Shift;
				const std::array<glm::vec2, 4> TexCoords = {
					glm::vec2(0.0f, 0.0f),
					glm::vec2(0.0f, 1.0f),
					glm::vec2(Frac, 1.0f),
					glm::vec2(Frac, 0.0f)};
				CRenderer::DrawQuad(TileCenter, glm::vec2(PartialLen, Chain.TextureHeight),
					*CRenderer::GetTexture(Texture), std::span<const glm::vec2, 4>(TexCoords), Color, AngleDeg);
			}
		}
	}

	void CScene::Render()
	{
		LK_PROFILER_SCOPED();
		for (const auto& Actor : Actors) {
			if (Actor->HasFlag(EActorFlag_Transparent)) {
				continue;
			}

			const CBody* Body = Actor->GetBody();
			if (Body == nullptr) {
				RenderActor(*Actor);
				continue;
			}

			std::visit([&]<typename T>(const T& ShapeRef)
			{
				if constexpr (std::is_same_v<T, FChain>) {
					RenderChain(*Actor, *Body, ShapeRef);
				} else {
					RenderActor(*Actor);
				}
			}, Body->GetShape());
		}
	}

	void CScene::RenderActor(const CActor& Actor)
	{
		const FTransformComponent& TC = Actor.GetTransformComponent();
		const float RotationDeg = glm::degrees(TC.GetRotation2D());
		const float OutlineThickness = Actor.IsOutlineEnabled() ? Actor.GetOutlineThickness() : 0.0f;

		const glm::vec2 BaseSize = glm::vec2(TC.Scale.x, TC.Scale.y);
		const glm::vec2 RenderSize = BaseSize * Actor.GetSpriteScale();

		/* Anchor the sprite at its bottom edge ('feet') so vertical scaling grows upward only. */
		glm::vec3 RenderPos = Actor.GetPosition();
		RenderPos.y += (RenderSize.y - BaseSize.y) * 0.50f;

		if (const CSprite* SpritePtr = Actor.GetSprite()) {
			CRenderer::DrawQuad(
				RenderPos,
				RenderSize,
				*CRenderer::GetTexture(Actor.GetTexture()),
				SpritePtr->GetUV(),
				Actor.GetColor(),
				RotationDeg,
				OutlineThickness,
				Actor.GetOutlineColor());
		} else {
			CRenderer::DrawQuad(
				RenderPos,
				RenderSize,
				Actor.GetTexture(),
				Actor.GetColor(),
				RotationDeg,
				OutlineThickness,
				Actor.GetOutlineColor());
		}
	}

	bool CScene::DoesActorExist(const LUUID Handle)
	{
		return GetActor(Handle) != nullptr;
	}

	bool CScene::DoesActorExist(std::string_view Name)
	{
		return GetActor(Name) != nullptr;
	}

	bool CScene::DeleteActor(const LUUID Handle)
	{
		LK_INFO_TAG("Scene", "Delete: {} (Actors: {})", Handle, Actors.size());
		auto IsHandleEqual = [Handle](const std::shared_ptr<CActor>& Actor)
		{
			return (Handle == Actor->GetHandle());
		};
		const std::size_t Erased = std::erase_if(Actors, IsHandleEqual);

		if (Erased > 0) {
			LK_ASSERT(Erased == 1, "Erased more than one actor");
			LK_DEBUG_TAG("Scene", "Successfully deleted: {} (Actors: {})", Handle, Actors.size());
			OnActorDeleted.Broadcast(Handle);
		}

		return (Erased == 1);
	}

	glm::mat4 CScene::GetWorldSpaceTransform(const LUUID ActorHandle)
	{
		glm::mat4 Transform(1.0f);
		std::shared_ptr<CActor> Actor = GetActor(ActorHandle);
		if (!Actor) {
			return Transform;
		}

		return (Transform * Actor->GetTransformComponent().GetTransform());
	}

	glm::mat4 CScene::GetWorldSpaceTransform(std::shared_ptr<CActor> Actor)
	{
		glm::mat4 Transform(1.0f);
		if (!Actor) {
			return Transform;
		}

		return (Transform * Actor->GetTransformComponent().GetTransform());
	}

	void CScene::SetName(std::string_view InName)
	{
		LK_DEBUG_TAG("Scene", "New name: {} (old: {})", InName, Name);
		Name = InName;
	}

	void CScene::SetState(const ESceneState InState)
	{
		State = InState;
	}

	bool CScene::Serialize(const std::filesystem::path& OutFile) const
	{
		std::filesystem::path SceneFile = OutFile;
		if (SceneFile.empty()) {
			SceneFile = Path;
		}
		LK_DEBUG_TAG("Scene", "Serialize: {}", StringUtils::GetPathRelativeToProject(SceneFile));

		YAML::Emitter Out;
		Out << YAML::BeginMap; /* Scene */
		Out << YAML::Key << "Name" << YAML::Value << Name;

		/* Actors */
		Out << YAML::Key << "Actors";
		Out << YAML::Value << YAML::BeginSeq;
		for (const auto& Actor : Actors) {
			Actor->Serialize(Out);
		}
		Out << YAML::EndSeq;
		/* ~Actors */

		Out << YAML::EndMap; /* ~Scene */

		/* Create scene directory if needed. */
		if (!std::filesystem::is_directory(SceneFile.parent_path())) {
			LK_WARN("Creating scenes directory as it was missing");
			const bool CreatedDirectory = std::filesystem::create_directories(SceneFile.parent_path());
			LK_VERIFY(CreatedDirectory, "Failed to create scenes directory");
		}

		std::ofstream File(SceneFile);
		if (!File.is_open()) {
			LK_ERROR_TAG("Scene", "File not open");
			return false;
		}

		File << Out.c_str();
		return true;
	}

	bool CScene::Deserialize(const std::filesystem::path& InFilepath)
	{
		LK_INFO_TAG("Scene", "Deserialize: {}", InFilepath);
		std::filesystem::path AbsFilepath;
		if (std::filesystem::exists(InFilepath)) {
			AbsFilepath = InFilepath;
		} else {
			AbsFilepath = Core::ProjectDir / InFilepath;
		}
		LK_TRACE_TAG("Scene", R"(Absolute filepath: "{}")", AbsFilepath);

		LK_ASSERT(std::filesystem::exists(AbsFilepath), R"(Filepath does not exist: "{}")", AbsFilepath);
		if (!std::filesystem::exists(AbsFilepath)) {
			LK_ERROR_TAG("Scene", R"(Filepath does not exist: "{}")", AbsFilepath);
			return false;
		}

		Path = AbsFilepath;

		/* Read YAML file to a string. */
		std::ifstream InputStream(AbsFilepath);
		std::stringstream StringStream;
		StringStream << InputStream.rdbuf();
		const std::string YamlString = StringStream.str();
		LK_ASSERT(!YamlString.empty(), R"(No YAML read from: "{}")", AbsFilepath);

		const YAML::Node Data = YAML::Load(YamlString);
		const std::string SceneName = Data["Name"].as<std::string>();
		Name = SceneName;
		LK_DEBUG_TAG("Scene", "Deserialized name: {}", Name);

		const YAML::Node ActorsNode = Data["Actors"];
		LK_ASSERT(!ActorsNode.IsNull());
		if (ActorsNode.IsNull()) {
			LK_ERROR_TAG("Scene", "Missing 'Actors' node in YAML");
			return false;
		}

		DeserializeActors(ActorsNode);

		return true;
	}

	static FActorSpecification ParseActorSpecification(const YAML::Node& Node)
	{
		using namespace Serialization;
		FActorSpecification ActorSpec;
		ActorSpec.Handle = Node["ID"].as<LUUID>();
		ActorSpec.Name = Node["Name"].as<std::string>();
		ActorSpec.Flags = static_cast<EActorFlag>(Node["Flags"].as<std::underlying_type_t<EActorFlag>>());
		ActorSpec.Type = static_cast<EActorType>(Node["Type"].as<std::size_t>());
		DeserializeProperty<Serialization::Optional>("SpriteScale", ActorSpec.SpriteScale, ActorSpec.SpriteScale, Node);

		const std::string TexturePath = Node["TexturePath"].as<std::string>();
		ActorSpec.Texture = CRenderer::GetTexture(TexturePath);
		ActorSpec.Color = Node["Color"].as<glm::vec4>();

		const YAML::Node& OutlineNode = Node["Outline"];
		DeserializeProperty("Enabled", ActorSpec.OutlineEnabled, true, OutlineNode);
		DeserializeProperty("Thickness", ActorSpec.OutlineThickness, 1.0f, OutlineNode);
		DeserializeProperty("Color", ActorSpec.OutlineColor, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), OutlineNode);

		return ActorSpec;
	}

	static std::optional<FBodySpecification> ParseBodySpecification(const YAML::Node& Node)
	{
		const YAML::Node BodyNode = Node["Body"];
		if (!BodyNode.IsDefined()) {
			LK_ERROR_TAG("Scene", "Body missing in YAML");
			return std::nullopt;
		}
		if (!BodyNode["Type"].IsDefined()) {
			return std::nullopt;
		}

		FBodySpecification BodySpec;
		Serialization::Deserialize(BodySpec, BodyNode);
		return BodySpec;
	}

	static std::vector<FSpawnWave> ParseSpawnWaves(const YAML::Node& SpawnerNode)
	{
		std::vector<FSpawnWave> Waves;
		const YAML::Node WavesNode = SpawnerNode["Waves"];
		if (!WavesNode.IsDefined() || !WavesNode.IsSequence()) {
			return Waves;
		}

		for (const YAML::Node& WaveNode : WavesNode) {
			FSpawnWave Wave;
			int ArchetypeValue = std::to_underlying(EEnemyArchetype::Grunt);
			Serialization::DeserializeProperty("Archetype", ArchetypeValue, ArchetypeValue, WaveNode);
			Wave.Archetype = static_cast<EEnemyArchetype>(ArchetypeValue);
			Serialization::DeserializeProperty("Count", Wave.Count, 3, WaveNode);
			Serialization::DeserializeProperty("SpawnInterval", Wave.SpawnInterval, 0.50f, WaveNode);
			Serialization::DeserializeProperty("StartDelay", Wave.StartDelay, 0.0f, WaveNode);
			Serialization::DeserializeProperty("WaitForClear", Wave.bWaitForClear, true, WaveNode);
			Waves.push_back(Wave);
		}
		return Waves;
	}

	static void AttachSerializedComponents(CActor& Actor, const YAML::Node& Node)
	{
		if (const YAML::Node TCNode = Node["TransformComponent"]; TCNode.IsDefined()) {
			FTransformComponent TC;
			Serialization::Deserialize(TC, TCNode);
			Actor.AddComponent<FTransformComponent>(TC);
		} else {
			LK_ERROR_TAG("Scene", "TransformComponent missing in YAML");
		}

		if (const YAML::Node EffectCompNode = Node["EffectComponent"]; EffectCompNode.IsDefined()) {
			FEffectComponent EC;
			Serialization::Deserialize(EC, EffectCompNode);
			LK_ASSERT(!EC.Effects.empty(), "At least one effect is required");
			Actor.AddComponent<FEffectComponent>(EC);
		}

		if (const YAML::Node InteractionCompNode = Node["InteractionComponent"]; InteractionCompNode.IsDefined()) {
			FInteractionComponent IC;
			Serialization::Deserialize(IC, InteractionCompNode);
			Actor.AddComponent<FInteractionComponent>(IC);
		}

		if (const YAML::Node HealthCompNode = Node["HealthComponent"]; HealthCompNode.IsDefined()) {
			FHealthComponent HC;
			Serialization::Deserialize(HC, HealthCompNode);
			Actor.AddComponent<FHealthComponent>(HC);
		}
	}

	std::shared_ptr<CActor> CScene::DeserializeEnemy(const YAML::Node& Node, const FActorSpecification& ActorSpec, FBodySpecification& BodySpec)
	{
		const YAML::Node& ControllerNode = Node["Controller"];
		LK_ASSERT(ControllerNode.IsDefined(), "Controller node missing");

		FEnemySpecification EnemySpec;
		Serialization::DeserializeProperty("ControllerType", EnemySpec.ControllerType, EControllerType::None, ControllerNode);
		Serialization::DeserializeProperty("SpawnPoint", EnemySpec.SpawnPoint, glm::vec2(0.0f, 0.0f), Node);
		Serialization::DeserializeProperty("Archetype", EnemySpec.Archetype, EEnemyArchetype::Grunt, Node);

		BodySpec.Flags |= EBodyFlag_ContactEvents;
		std::shared_ptr<CEnemy> Enemy = Create<CEnemy>(EnemySpec, ActorSpec, BodySpec);
		Enemy->SetPosition(BodySpec.Position);

		if (EnemySpec.ControllerType == EControllerType::Patrol) {
			float HalfDistance = 0.0f;
			float StartDelayInSeconds = 0.0f;
			Serialization::DeserializeProperty("HalfDistance", HalfDistance, 0.0f, ControllerNode);
			Serialization::DeserializeProperty("StartDelayInSeconds", StartDelayInSeconds, 0.0f, ControllerNode);
			Enemy->SetController(std::make_unique<CPatrolController>(1.0f, 1.0f));
		}

		return Enemy;
	}

	std::shared_ptr<CActor> CScene::DeserializeSpawner(const YAML::Node& Node, const FActorSpecification& ActorSpec)
	{
		std::shared_ptr<CEnemySpawner> SpawnerActor = Create<CEnemySpawner>(ActorSpec);

		const YAML::Node SpawnerNode = Node["Spawner"];
		if (!SpawnerNode.IsDefined()) {
			return SpawnerActor;
		}

		int MaxAlive = 8;
		bool Loop = false;
		float Scatter = 0.25f;
		bool AutoActivate = false;
		Serialization::DeserializeProperty("MaxAlive", MaxAlive, 8, SpawnerNode);
		Serialization::DeserializeProperty("Loop", Loop, false, SpawnerNode);
		Serialization::DeserializeProperty("Scatter", Scatter, 0.25f, SpawnerNode);
		Serialization::DeserializeProperty("AutoActivate", AutoActivate, false, SpawnerNode);
		SpawnerActor->SetMaxAlive(MaxAlive);
		SpawnerActor->SetLoop(Loop);
		SpawnerActor->SetScatter(Scatter);
		SpawnerActor->SetAutoActivate(AutoActivate);
		SpawnerActor->SetWaves(ParseSpawnWaves(SpawnerNode));

		return SpawnerActor;
	}

	std::shared_ptr<CActor> CScene::DeserializeActor(const YAML::Node& Node)
	{
		LK_ASSERT(Node["ID"] && Node["Name"] && Node["Flags"] && Node["TexturePath"] && Node["Color"] && Node["TransformComponent"]);

		const FActorSpecification ActorSpec = ParseActorSpecification(Node);
		LK_TRACE_TAG("Scene", "Deserialize: {} ({})", ActorSpec.Name, ActorSpec.Handle);
		LK_VERIFY(!DoesActorExist(ActorSpec.Handle), "Duplicate actors found with handle {}", ActorSpec.Handle);

		std::optional<FBodySpecification> BodySpec = ParseBodySpecification(Node);
		const bool HasBody = BodySpec.has_value();

		std::shared_ptr<CActor> Actor = nullptr;
		switch (ActorSpec.Type) {
			case EActorType::Object:
				[[fallthrough]];
			case EActorType::Spawnpoint:
				Actor = HasBody ? Create<CActor>(ActorSpec, *BodySpec) : Create<CActor>(ActorSpec);
				break;
			case EActorType::Player:
				LK_VERIFY(HasBody, "Body is expected for {}: {} ({})", Enum::ToString(ActorSpec.Type), ActorSpec.Handle, ActorSpec.Name);
				Actor = Create<CActor>(ActorSpec, *BodySpec);
				break;
			case EActorType::Enemy:
				LK_VERIFY(HasBody, "Body is expected for {}: {} ({})", Enum::ToString(ActorSpec.Type), ActorSpec.Handle, ActorSpec.Name);
				Actor = DeserializeEnemy(Node, ActorSpec, *BodySpec);
				break;
			case EActorType::Spawner:
				Actor = DeserializeSpawner(Node, ActorSpec);
				break;
			default:
				break;
		}

		LK_VERIFY(Actor, "Actor is NULL, {} not supported: {} ({})", Enum::ToString(ActorSpec.Type), ActorSpec.Handle, ActorSpec.Name);
		AttachSerializedComponents(*Actor, Node);

		return Actor;
	}

	void CScene::DeserializeActors(const YAML::Node& ActorsNode)
	{
		LK_DEBUG_TAG("Scene", "Deserializing actors");
		for (const YAML::Node& Node : ActorsNode) {
			DeserializeActor(Node);
		}
	}

}
