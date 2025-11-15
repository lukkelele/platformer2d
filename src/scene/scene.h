#pragma once

#include "core/core.h"
#include "actor.h"

namespace platformer2d {

	enum class ESceneState
	{
		Edit,
		Play,
		Pause,
		Simulate
	};

	class CScene : public ISerializable<ESerializable::File>
	{
	public:
		CScene(std::string_view InName);
		CScene() = delete;
		~CScene();

		void Tick(float DeltaTime);

		std::shared_ptr<CActor> FindActor(LUUID Handle);
		std::shared_ptr<CActor> FindActor(std::string_view Name);
		bool DoesActorExist(LUUID Handle);
		bool DoesActorExist(std::string_view Name);
		bool DeleteActor(LUUID Handle);

		glm::mat4 GetWorldSpaceTransform(LUUID ActorHandle);
		glm::mat4 GetWorldSpaceTransform(std::shared_ptr<CActor> Actor);

		/* @fixme: Temporary fix until rendering is supported entirely from within the class. */
		FORCEINLINE const std::vector<std::shared_ptr<CActor>>& GetActors() const { return Actors; }

		std::string_view GetName() const { return Name; }
		void SetName(std::string_view InName);
		const std::filesystem::path& GetFilepath() const { return Filepath; }

		virtual bool Serialize(const std::filesystem::path& OutFile = {}) const override;
		virtual bool Deserialize(const std::filesystem::path& InFile) override;

	private:
		void DeserializeActors(const YAML::Node& ActorsNode);

	public:
		static constexpr const char* FILE_EXTENSION = "lscene";
	private:
		LUUID ID;
		std::string Name;
		std::filesystem::path Filepath;
		std::vector<std::shared_ptr<CActor>> Actors{};

		bool bPaused = false;
	};

	namespace Enum
	{
		inline const char* ToString(const ESceneState State)
		{
			const char* S = "";
		#define _(EnumValue) case ESceneState::EnumValue: S = #EnumValue; break
			switch (State)
			{
				_(Edit);
				_(Play);
				_(Pause);
				_(Simulate);
				default:
					LK_THROW_ENUM_ERR(State);
					break;
			}
		#undef _
			return S;
		}
	}


}
