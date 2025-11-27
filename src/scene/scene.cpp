#include "scene.h"

#include <filesystem>
#include <fstream>
#include <istream>

#include "core/string.h"
#include "game/instance.h"
#include "serialization/serialization.h"

namespace platformer2d {

	CScene::CScene(std::string_view InName)
		: Name(InName)
	{
		LK_VERIFY(!Name.empty(), "Scene name is empty");
		Path = LK_FMT("{}/{}.{}", SCENES_DIR, Name, FILE_EXTENSION).c_str();
		LK_DEBUG_TAG("Scene", "Created: {} ({})", Name, StringUtils::GetPathRelativeToProject(Path));

		OnActorDeleted.Add([&](const LUUID Handle)
		{
			LK_DEBUG_TAG("Scene", "OnActorDeleted: {}", Handle);
			if (CGameInstance* GameInstance = CGameInstance::Get(); GameInstance != nullptr)
			{
				CPlayer* Player = GameInstance->GetPlayer(0);
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
		for (const auto& Actor : Actors)
		{
			Actor->Tick(DeltaTime);
		}
	}

	std::shared_ptr<CActor> CScene::FindActor(const LUUID Handle)
	{
		auto IsHandleEqual = [Handle](const std::shared_ptr<CActor>& Actor)
		{
			return (Handle == Actor->GetHandle());
		};
		auto Iter = std::find_if(Actors.begin(), Actors.end(), IsHandleEqual);
		return (Iter != Actors.end()) ? *Iter : nullptr;
	}

	std::shared_ptr<CActor> CScene::FindActor(std::string_view Name)
	{
		auto IsNameEqual = [Name](const std::shared_ptr<CActor>& Actor)
		{
			return (Name == Actor->GetName());
		};
		auto Iter = std::find_if(Actors.begin(), Actors.end(), IsNameEqual);
		return (Iter != Actors.end()) ? *Iter : nullptr;
	}

	bool CScene::DoesActorExist(const LUUID Handle)
	{
		return FindActor(Handle) != nullptr;
	}

	bool CScene::DoesActorExist(std::string_view Name)
	{
		return FindActor(Name) != nullptr;
	}

	bool CScene::DeleteActor(const LUUID Handle)
	{
		LK_INFO_TAG("Scene", "Delete: {} (Actors: {})", Handle, Actors.size());
		auto IsHandleEqual = [Handle](const std::shared_ptr<CActor>& Actor)
		{
			return (Handle == Actor->GetHandle());
		};
		const std::size_t Erased = std::erase_if(Actors, IsHandleEqual);

		if (Erased > 0)
		{
			LK_ASSERT(Erased == 1, "Erased more than one actor");
			LK_DEBUG_TAG("Scene", "Successfully deleted: {} (Actors: {})", Handle, Actors.size());
			OnActorDeleted.Broadcast(Handle);
		}

		return (Erased == 1);
	}

	glm::mat4 CScene::GetWorldSpaceTransform(const LUUID ActorHandle)
	{
		glm::mat4 Transform(1.0f);
		std::shared_ptr<CActor> Actor = FindActor(ActorHandle);
		if (Actor == nullptr)
		{
			return Transform;
		}

		return (Transform * Actor->GetTransformComponent().GetTransform());
	}

	glm::mat4 CScene::GetWorldSpaceTransform(std::shared_ptr<CActor> Actor)
	{
		glm::mat4 Transform(1.0f);
		if (Actor == nullptr)
		{
			return Transform;
		}

		return (Transform * Actor->GetTransformComponent().GetTransform());
	}

	void CScene::SetName(std::string_view InName)
	{
		LK_DEBUG_TAG("Scene", "New name: {} (old: {})", InName, Name);
		Name = InName;
	}

	bool CScene::Serialize(const std::filesystem::path& OutFile) const
	{
		std::filesystem::path SceneFile = OutFile;
		if (SceneFile.empty())
		{
			SceneFile = Path;
		}
		LK_DEBUG_TAG("Scene", "Serialize: {}", StringUtils::GetPathRelativeToProject(SceneFile));

		YAML::Emitter Out;
		Out << YAML::BeginMap; /* Scene */
		Out << YAML::Key << "Name" << YAML::Value << Name;

		/* Actors */
		Out << YAML::Key << "Actors";
		Out << YAML::Value << YAML::BeginSeq;
		for (const auto& Actor : Actors)
		{
			Actor->Serialize(Out);
		}
		Out << YAML::EndSeq;
		/* ~Actors */

		Out << YAML::EndMap; /* ~Scene */

		/* Create scene directory if needed. */
		if (!std::filesystem::is_directory(SceneFile.parent_path()))
		{
			LK_WARN("Creating scenes directory as it was missing");
			const bool CreatedDirectory = std::filesystem::create_directories(SceneFile.parent_path());
			LK_VERIFY(CreatedDirectory, "Failed to create scenes directory");
		}

		std::ofstream File(SceneFile);
		if (!File.is_open())
		{
			LK_ERROR_TAG("Scene", "File not open");
			return false;
		}

		File << Out.c_str();
		return true;
	}

	bool CScene::Deserialize(const std::filesystem::path& InFilepath)
	{
		LK_INFO_TAG("Scene", "Deserialize: {}", InFilepath);
		std::filesystem::path AbsFilepath = Core::ProjectDir / InFilepath;
		LK_TRACE_TAG("Scene", "Absolute filepath: {}", AbsFilepath);
		LK_ASSERT(std::filesystem::exists(AbsFilepath), "Filepath does not exist: {}", AbsFilepath);
		if (!std::filesystem::exists(AbsFilepath))
		{
			LK_ERROR_TAG("Scene", "Filepath does not exist: {}", AbsFilepath);
			return false;
		}

		Path = AbsFilepath;

		/* Read YAML file to a string. */
		std::ifstream InputStream(AbsFilepath);
		std::stringstream StringStream;
		StringStream << InputStream.rdbuf();
		const std::string YamlString = StringStream.str();

		const YAML::Node Data = YAML::Load(YamlString);
		const std::string SceneName = Data["Name"].as<std::string>();
		Name = SceneName;
		LK_DEBUG_TAG("Scene", "Deserialized name: {}", Name);

		const YAML::Node ActorsNode = Data["Actors"];
		LK_ASSERT(!ActorsNode.IsNull());
		if (ActorsNode.IsNull())
		{
			LK_ERROR_TAG("Scene", "Missing 'Actors' node in YAML");
			return false;
		}

		DeserializeActors(ActorsNode);

		return true;
	}

	void CScene::DeserializeActors(const YAML::Node& ActorsNode)
	{
		LK_DEBUG_TAG("Scene", "Deserializing actors");
		for (const YAML::Node& Node : ActorsNode)
		{
			LK_ASSERT(Node["ID"] && Node["Name"] && Node["Texture"] && Node["Color"] && Node["TransformComponent"]);
			const LUUID ActorHandle = Node["ID"].as<LUUID>();
			const std::string ActorName = Node["Name"].as<std::string>();
			const ETexture ActorTexture = static_cast<ETexture>(Node["Texture"].as<int>());
			const glm::vec4 ActorColor = Node["Color"].as<glm::vec4>();
			LK_TRACE_TAG("Scene", "Deserialize: {} ({})", ActorName, ActorHandle);

			FTransformComponent TC;
			if (const YAML::Node TCNode = Node["TransformComponent"]; TCNode.IsDefined())
			{
				Serialization::Deserialize(TC, TCNode);
			}
			else
			{
				LK_WARN_TAG("Scene", "TransformComponent missing in YAML");
			}

			FBodySpecification BodySpec;
			BodySpec.Name = ActorName;
			if (const YAML::Node BodyNode = Node["Body"]; BodyNode.IsDefined())
			{
				Serialization::Deserialize(BodySpec, BodyNode);
			}
			else
			{
				LK_WARN_TAG("Scene", "Body missing in YAML");
			}

			bool HasEffectComponent = false;
			FEffectComponent EC;
			if (const YAML::Node EffectCompNode = Node["EffectComponent"]; EffectCompNode.IsDefined())
			{
				HasEffectComponent = true;
				Serialization::Deserialize(EC, EffectCompNode);
				LK_ASSERT(!EC.Effects.empty(), "At least one effect is required");
			}

			if (!DoesActorExist(ActorHandle))
			{
				std::shared_ptr<CActor> Actor = Create<CActor>(ActorHandle, BodySpec, ActorTexture, ActorColor);
				Actor->GetTransformComponent() = TC;

				if (HasEffectComponent)
				{
					Actor->AddComponent<FEffectComponent>(EC);
				}
			}
			else
			{
				LK_ERROR_TAG("Scene", "Duplicate actors found with handle {}", ActorHandle);
			}
		}
	}

}
