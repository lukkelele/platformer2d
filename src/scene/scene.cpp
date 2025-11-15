#include "scene.h"

#include <fstream>
#include <istream>

#include "core/string.h"
#include "game/gameinstance.h"
#include "serialization/serialization.h"

namespace platformer2d {

	CScene::CScene(std::string_view InName)
		: Name(InName)
	{
		LK_VERIFY(!Name.empty(), "Scene name is empty");
		Filepath = LK_FMT("{}/{}.{}", SCENES_DIR, Name, FILE_EXTENSION).c_str();
		LK_DEBUG_TAG("Scene", "Created: {} ({})", Name, StringUtils::GetPathRelativeToProject(Filepath));

		CActor::OnActorCreated.Add([&](const LUUID Handle, std::weak_ptr<CActor> ActorRef)
		{
			if (std::shared_ptr<CActor> Actor = ActorRef.lock(); Actor != nullptr)
			{
				LK_TRACE_TAG("Scene", "OnActorCreated: {} ({})", Actor->GetName(), Handle);
				Actors.emplace_back(Actor);

				/* @fixme */
#if 0
				std::string_view ActorName = Actor->GetName();
				if (ActorName.find("Rotating") != std::string::npos)
				{
					/* @fixme: Temporary fix until serialization can take effect parameters. */
					RotatingPlatform = Actor;
				}
#endif
			}
		});

		/**
		 * Bind to lambda because the delegate expects a void return type,
		 * which DeleteActor isn't.
		 */
		CActor::OnActorMarkedForDeletion.Add([&](const LUUID Handle)
		{
			/* Move the player a little bit to cause physics to pass through. */
			if (DeleteActor(Handle))
			{
				if (IGameInstance* GameInstance = IGameInstance::Get(); GameInstance != nullptr)
				{
					CPlayer* Player = GameInstance->GetPlayer(0);
					/* @todo: Instead of applied force, try to just send wake-up event */
					Player->GetBody().ApplyForce({ 0.0f, 0.010f });
				}
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

		/* @fixme */
#if 0
		if (std::shared_ptr<CActor> Actor = RotatingPlatform.lock(); Actor != nullptr)
		{
			Actor->SetRotation(Actor->GetRotation() + glm::radians(0.75f));
		}
#endif
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
		LK_INFO_TAG("Scene", "[{}] Delete: {}", Name, Handle);
		auto IsHandleEqual = [Handle](const std::shared_ptr<CActor>& Actor)
		{
			return (Handle == Actor->GetHandle());
		};
		const std::size_t Erased = std::erase_if(Actors, IsHandleEqual);
		LK_ASSERT(Erased == 1, "Erased={}", Erased);
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
			SceneFile = Filepath;
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

	bool CScene::Deserialize(const std::filesystem::path& Filepath)
	{
		LK_INFO_TAG("Scene", "Deserialize: {}", Filepath);
		LK_ASSERT(std::filesystem::exists(Filepath), "Filepath does not exist: {}", Filepath);
		if (!std::filesystem::exists(Filepath))
		{
			LK_ERROR_TAG("Scene", "Filepath does not exist: {}", Filepath);
			return false;
		}

		std::ifstream InputStream(Filepath);
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
			if (const YAML::Node TCNode = Node["TransformComponent"]; !TCNode.IsNull())
			{
				Serialization::Deserialize(TC, TCNode);
			}
			else
			{
				LK_WARN_TAG("Scene", "TransformComponent missing in YAML");
			}

			FBodySpecification BodySpec;
			BodySpec.Name = ActorName;
			if (const YAML::Node BodyNode = Node["Body"]; !BodyNode.IsNull())
			{
				Serialization::Deserialize(BodySpec, BodyNode);
			}
			else
			{
				LK_WARN_TAG("Scene", "Body missing in YAML");
			}

			if (!DoesActorExist(ActorHandle))
			{
				std::shared_ptr<CActor> Actor = CActor::Create<CActor>(ActorHandle, BodySpec, ActorTexture, ActorColor);
			}
			else
			{
				LK_ERROR_TAG("Scene", "Duplicate actors found with handle {}", ActorHandle);
			}
		}
	}

}
