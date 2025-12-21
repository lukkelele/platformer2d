#pragma once

#include "core/core.h"
#include "actor.h"

namespace platformer2d {

	enum class ESceneState
	{
		None,
		Play,
		Pause,
		Edit,
		COUNT
	};

	class CScene : public ISerializable<ESerializable::File>
	{
	public:
		LK_DECLARE_EVENT(FOnActorCreated, CScene, LUUID, std::weak_ptr<CActor>);
		LK_DECLARE_EVENT(FOnActorDeleted, CScene, LUUID);
	public:
		CScene(std::string_view InName);
		CScene() = delete;
		~CScene();

		void Tick(float DeltaTime);
		void Render();

		template<typename T, typename... TArgs>
		std::shared_ptr<T> Create(TArgs&&... Args)
		{
			static_assert(std::is_base_of_v<CActor, T>);
			std::shared_ptr<T> Actor = std::shared_ptr<T>(new T(std::forward<TArgs>(Args)...));
			if (Actor != nullptr)
			{
				LK_DEBUG_TAG("Scene", "Created: {} ({})", Actor->GetName(), Actor->GetHandle());
				Actors.emplace_back(Actor);

				OnActorCreated.Broadcast(Actor->GetHandle(), Actor);
			}

			return Actor;
		}

		template<typename T = CActor>
		std::shared_ptr<T> FindActor(const LUUID Handle)
		{
			auto IsHandleEqual = [Handle](const std::shared_ptr<CActor>& Actor)
			{
				return (Handle == Actor->GetHandle());
			};
			auto Iter = std::find_if(Actors.begin(), Actors.end(), IsHandleEqual);
			if constexpr (std::is_same_v<T, CActor>) {
				return (Iter != Actors.end()) ? *Iter : nullptr;
			} else {
				return (Iter != Actors.end()) ? std::static_pointer_cast<T>(*Iter) : nullptr;
			}
		}

		template<typename T = CActor>
		std::shared_ptr<T> FindActor(std::string_view Name)
		{
			auto IsNameEqual = [Name](const std::shared_ptr<CActor>& Actor)
			{
				return (Name == Actor->GetName());
			};
			auto Iter = std::find_if(Actors.begin(), Actors.end(), IsNameEqual);
			return (Iter != Actors.end()) ? std::static_pointer_cast<T>(*Iter) : nullptr;
		}

		bool DoesActorExist(LUUID Handle);
		bool DoesActorExist(std::string_view Name);
		bool DeleteActor(LUUID Handle);

		glm::mat4 GetWorldSpaceTransform(LUUID ActorHandle);
		glm::mat4 GetWorldSpaceTransform(std::shared_ptr<CActor> Actor);

		/* @fixme: Temporary fix until rendering is supported entirely from within the class. */
		FORCEINLINE const std::vector<std::shared_ptr<CActor>>& GetActors() const { return Actors; }

		std::string_view GetName() const { return Name; }
		void SetName(std::string_view InName);
		void SetState(ESceneState InState);
		ESceneState GetState() const { return State; }
		const std::filesystem::path& GetFilepath() const { return Path; }

		virtual bool Serialize(const std::filesystem::path& OutFile = {}) const override;
		virtual bool Deserialize(const std::filesystem::path& InFile) override;

	private:
		void DeserializeActors(const YAML::Node& ActorsNode);

	public:
		static constexpr const char* FILE_EXTENSION = "lscene";
		static inline FOnActorCreated OnActorCreated;
		static inline FOnActorDeleted OnActorDeleted;
	private:
		LUUID ID;
		std::string Name;
		ESceneState State = ESceneState::None;
		std::filesystem::path Path;
		std::vector<std::shared_ptr<CActor>> Actors{};
	};

	namespace Enum {
		inline const char* ToString(const ESceneState State)
		{
			const char* S = "";
		#define _(EnumValue) case ESceneState::EnumValue: S = #EnumValue; break
			switch (State) {
				_(None);
				_(Play);
				_(Pause);
				_(Edit);
				_(COUNT);
				default:
					LK_THROW_ENUM_ERR(State);
					break;
			}
		#undef _
			return S;
		}
	}

}
