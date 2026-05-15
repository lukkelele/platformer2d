#include "scene.h"

#include <filesystem>
#include <fstream>
#include <istream>

#include "core/profiler.h"
#include "core/string.h"
#include "game/enemy.h"
#include "game/instance.h"
#include "game/controller/patrolcontroller.h"
#include "renderer/renderer.h"
#include "serialization/serialization.h"

namespace platformer2d {

	static void RenderQuad(const CActor& Actor)
	{
		const FTransformComponent& TC = Actor.GetTransformComponent();
		CRenderer::DrawQuad(
			Actor.GetPosition(),
			TC.Scale,
			Actor.GetTexture(),
			Actor.GetColor(),
			glm::degrees(TC.GetRotation2D()),
			Actor.IsOutlineEnabled() ? Actor.GetOutlineThickness() : 0.0f,
			Actor.GetOutlineColor());
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
		if (Textured) {
			for (std::size_t Idx = 0; Idx < Last; Idx++) {
				const glm::vec2 P0 = Origin + Chain.Points[Idx];
				const glm::vec2 P1 = Origin + Chain.Points[(Idx + 1) % Count];
				const glm::vec2 Center = (P0 + P1) * 0.50f;
				const float AngleDeg = glm::degrees(std::atan2(P1.y - P0.y, P1.x - P0.x));
				CRenderer::DrawQuad(Center, glm::vec2(glm::length(P1 - P0), Chain.TextureHeight), Texture, Color, AngleDeg);
			}
		} else {
			for (std::size_t Idx = 0; Idx < Last; Idx++) {
				const glm::vec2 P0 = Origin + Chain.Points[Idx];
				const glm::vec2 P1 = Origin + Chain.Points[(Idx + 1) % Count];
				CRenderer::DrawLine(P0, P1, Color, 4);
			}
		}
	}

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
		LK_PROFILE_FUNC();
		for (const auto& Actor : Actors) {
			Actor->Tick(DeltaTime);
		}
	}

	void CScene::Render()
	{
		LK_PROFILE_FUNC();
		for (const auto& Actor : Actors) {
			if (Actor->HasFlag(EActorFlag_Transparent)) {
				continue;
			}

			const CBody* Body = Actor->GetBody();
			if (Body == nullptr) {
				RenderQuad(*Actor);
				continue;
			}

			std::visit([&]<typename T>(const T& ShapeRef)
			{
				if constexpr (std::is_same_v<T, FChain>) {
					RenderChain(*Actor, *Body, ShapeRef);
				} else {
					RenderQuad(*Actor);
				}
			}, Body->GetShape());
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

	void CScene::DeserializeActors(const YAML::Node& ActorsNode)
	{
		LK_DEBUG_TAG("Scene", "Deserializing actors");
		for (const YAML::Node& Node : ActorsNode) {
			LK_ASSERT(Node["ID"] && Node["Name"] && Node["Texture"] && Node["Color"] && Node["TransformComponent"]);
			FActorSpecification ActorSpec;
			ActorSpec.Handle = Node["ID"].as<LUUID>();
			ActorSpec.Name = Node["Name"].as<std::string>();
			const EActorType ActorType = static_cast<EActorType>(Node["Type"].as<std::size_t>());
			ActorSpec.Type = ActorType;
			LK_TRACE_TAG("Scene", "Deserialize: {} ({})", ActorSpec.Name, ActorSpec.Handle);

			ActorSpec.Texture = static_cast<ETexture>(Node["Texture"].as<int>());
			ActorSpec.Color = Node["Color"].as<glm::vec4>();

			const YAML::Node& OutlineNode = Node["Outline"];
			Serialization::DeserializeProperty("Enabled", ActorSpec.OutlineEnabled, true, OutlineNode);
			Serialization::DeserializeProperty("Thickness", ActorSpec.OutlineThickness, 1.0f, OutlineNode);
			Serialization::DeserializeProperty("Color", ActorSpec.OutlineColor, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), OutlineNode);

			std::optional<FTransformComponent> TC;
			if (const YAML::Node TCNode = Node["TransformComponent"]; TCNode.IsDefined()) {
				TC.emplace();
				Serialization::Deserialize(*TC, TCNode);
			} else {
				LK_ERROR_TAG("Scene", "TransformComponent missing in YAML");
			}

			std::optional<FBodySpecification> BodySpec;
			if (const YAML::Node BodyNode = Node["Body"]; BodyNode.IsDefined()) {
				if (BodyNode["Type"].IsDefined()) {
					BodySpec.emplace();
					Serialization::Deserialize(*BodySpec, BodyNode);
				} else {
					LK_DEBUG_TAG("Scene", "Actor {} has no body", ActorSpec.Name);
				}
			} else {
				LK_ERROR_TAG("Scene", "Body missing in YAML");
			}

			std::optional<FEffectComponent> EC;
			if (const YAML::Node EffectCompNode = Node["EffectComponent"]; EffectCompNode.IsDefined()) {
				EC.emplace();
				Serialization::Deserialize(*EC, EffectCompNode);
				LK_ASSERT(!(*EC).Effects.empty(), "At least one effect is required");
			}

			std::optional<FInteractionComponent> IC;
			if (const YAML::Node InteractionCompNode = Node["InteractionComponent"]; InteractionCompNode.IsDefined()) {
				IC.emplace();
				Serialization::Deserialize(*IC, InteractionCompNode);
			}

			std::optional<FHealthComponent> HC;
			if (const YAML::Node HealthCompNode = Node["HealthComponent"]; HealthCompNode.IsDefined()) {
				HC.emplace();
				Serialization::Deserialize(*HC, HealthCompNode);
			}

			std::optional<FEnemySpecification> EnemySpec;
			if (ActorType == EActorType::Enemy) {
				const YAML::Node& ControllerNode = Node["Controller"];
				LK_ASSERT(ControllerNode.IsDefined(), "Controller node missing");
				auto& Spec = EnemySpec.emplace();
				Serialization::DeserializeProperty("ControllerType", Spec.ControllerType, EControllerType::None, ControllerNode);
				Serialization::DeserializeProperty("SpawnPoint", Spec.SpawnPoint, glm::vec2(0.0f, 0.0f), Node);
				/* @fixme: Temporarily optional */
				Serialization::DeserializeProperty<Serialization::EProperty::Optional>("Archetype", Spec.Archetype, EEnemyArchetype::Grunt, Node);
			}

			LK_VERIFY(!DoesActorExist(ActorSpec.Handle), "Duplicate actors found with handle {}", ActorSpec.Handle);

			std::shared_ptr<CActor> Actor = nullptr;
			const bool HasBody = BodySpec.has_value();
			switch (ActorType) {
				case EActorType::Object:
					[[fallthrough]];
				case EActorType::Spawnpoint:
				{
					if (HasBody) {
						Actor = Create<CActor>(ActorSpec, *BodySpec);
					} else {
						Actor = Create<CActor>(ActorSpec);
					}
					break;
				}
				case EActorType::Player:
				{
					LK_VERIFY(HasBody, "Body is expected for {}: {} ({})", Enum::ToString(ActorType), ActorSpec.Handle, ActorSpec.Name);
					Actor = Create<CActor>(ActorSpec, *BodySpec);
					break;
				}
				case EActorType::Enemy:
				{
					LK_VERIFY(HasBody, "Body is expected for {}: {} ({})", Enum::ToString(ActorType), ActorSpec.Handle, ActorSpec.Name);
					LK_VERIFY(EnemySpec.has_value(), "Enemy specification missing for {} ({})", ActorSpec.Handle, ActorSpec.Name);
					Actor = Create<CEnemy>(*EnemySpec, ActorSpec, *BodySpec);

					/* Create controller for the enemy. */
					const FEnemySpecification& Spec = *EnemySpec;
					if (Spec.ControllerType == EControllerType::Patrol) {
						const YAML::Node& ControllerNode = Node["Controller"];
						float HalfDistance = 0.0f;
						float StartDelayInSeconds = 0.0f;
						Serialization::DeserializeProperty("HalfDistance", HalfDistance, 0.0f, ControllerNode);
						Serialization::DeserializeProperty("StartDelayInSeconds", StartDelayInSeconds, 0.0f, ControllerNode);
						Actor->As<CEnemy>().SetController(std::make_unique<CPatrolController>(1.0f, 1.0f));
					}
					break;
				}
				default:
					break;
			}

			LK_VERIFY(Actor, "Actor is NULL, {} not supported: {} ({})", Enum::ToString(ActorType), ActorSpec.Handle, ActorSpec.Name);
			if (TC.has_value()) {
				Actor->AddComponent<FTransformComponent>(*TC);
			}
			if (EC.has_value()) {
				Actor->AddComponent<FEffectComponent>(*EC);
			}
			if (IC.has_value()) {
				Actor->AddComponent<FInteractionComponent>(*IC);
			}
			if (HC.has_value()) {
				Actor->AddComponent<FHealthComponent>(*HC);
			}
		}
	}

}
