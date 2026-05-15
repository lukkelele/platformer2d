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
	LK_ENUM(ESceneState);

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

		static void RenderActor(const CActor& Actor);

		template<typename T, typename... TArgs>
		[[nodiscard]] std::shared_ptr<T> Create(TArgs&&... Args)
		{
			static_assert(std::is_base_of_v<CActor, T>);
			std::shared_ptr<T> Actor = std::shared_ptr<T>(new T(std::forward<TArgs>(Args)...));
			if (Actor != nullptr) {
				LK_DEBUG_TAG("Scene", "Created: {} ({})", Actor->GetName(), Actor->GetHandle());
				Actors.emplace_back(Actor);
				OnActorCreated.Broadcast(Actor->GetHandle(), Actor);
			}
			return Actor;
		}

		template<typename T = CActor>
		[[nodiscard]] std::shared_ptr<T> GetActor(const LUUID Handle)
		{
			auto IsHandleEqual = [Handle](const std::shared_ptr<CActor>& Actor)
			{
				return (Handle == Actor->GetHandle());
			};
			auto Iter = std::find_if(Actors.begin(), Actors.end(), IsHandleEqual);
			return (Iter != Actors.end()) ? std::static_pointer_cast<T>(*Iter) : nullptr;
		}

		template<typename T = CActor>
		[[nodiscard]] std::shared_ptr<T> GetActor(std::string_view Name)
		{
			auto IsNameEqual = [Name](const std::shared_ptr<CActor>& Actor)
			{
				return (Name == Actor->GetName());
			};
			auto Iter = std::find_if(Actors.begin(), Actors.end(), IsNameEqual);
			return (Iter != Actors.end()) ? std::static_pointer_cast<T>(*Iter) : nullptr;
		}

		template<typename T>
		std::size_t GetAllOfType(std::vector<std::shared_ptr<T>>& Container)
		{
			Container.clear();
			Container.reserve(Actors.size());

			for (const auto& Actor : Actors) {
				if (!Actor) {
					continue;
				}

				/* @todo: Use dynamic casting until derived class type can be used for runtime checks */
				if (const auto Casted = std::dynamic_pointer_cast<T>(Actor)) {
					Container.push_back(Casted);
				}
			}

			return Container.size();
		}

		template<typename T>
		[[nodiscard]] std::vector<std::shared_ptr<T>> GetAllOfType()
		{
			std::vector<std::shared_ptr<T>> Result;
			GetAllOfType<T>(Result);

			return Result;
		}

		bool DoesActorExist(LUUID Handle);
		bool DoesActorExist(std::string_view Name);
		bool DeleteActor(LUUID Handle);
		[[nodiscard]] const std::vector<std::shared_ptr<CActor>>& GetActors() const { return Actors; }

		[[nodiscard]] glm::mat4 GetWorldSpaceTransform(LUUID ActorHandle);
		[[nodiscard]] glm::mat4 GetWorldSpaceTransform(std::shared_ptr<CActor> Actor);

		[[nodiscard]] ESceneState GetState() const { return State; }
		[[nodiscard]] std::string_view GetName() const { return Name; }
		void SetName(std::string_view InName);
		void SetState(ESceneState InState);
		[[nodiscard]] const std::filesystem::path& GetFilepath() const { return Path; }

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

}

