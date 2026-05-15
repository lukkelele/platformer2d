#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include <glm/glm.hpp>

#include "core/core.h"
#include "core/delegate.h"
#include "gamesystem.h"

namespace platformer2d {

	class CPlayer;
	class IItem;

	struct FCheckpointPreview
	{
		bool bExists = false;
		std::string CurrentID;
		std::size_t TriggeredCount = 0;
	};

	class CCheckpointSystem : public IGameSystem
	{
	public:
		LK_DECLARE_MULTICAST_DELEGATE(FOnSaved, std::string_view);
		FOnSaved OnSaved;

	public:
		CCheckpointSystem() = default;
		CCheckpointSystem(CCheckpointSystem&&) = delete;
		CCheckpointSystem(const CCheckpointSystem&) = delete;
		~CCheckpointSystem() = default;

		CCheckpointSystem& operator=(CCheckpointSystem&&) = delete;
		CCheckpointSystem& operator=(const CCheckpointSystem&) = delete;

		void Initialize(CGameInstance& Owner) override;
		void Shutdown() override;

		bool TrySave(CPlayer& Player, std::string_view CheckpointID, const std::filesystem::path& ScenePath);
		bool RestoreToPlayer(CPlayer& Player);
		[[nodiscard]] bool HasCheckpoint() const;

		bool LoadFromDisk(const std::filesystem::path& LevelFilepath);
		bool SaveToDisk();
		void Clear();

		std::string_view GetCurrentID() const;

		[[nodiscard]] static FCheckpointPreview PeekFromDisk(const std::filesystem::path& LevelFilepath);
		[[nodiscard]] static std::filesystem::path DeriveCheckpointPath(const std::filesystem::path& LevelFilepath);

	private:

	private:
		struct FState
		{
			std::filesystem::path LevelFilepath;
			std::filesystem::path ScenePath;
			std::string CurrentID;
			glm::vec2 Position = {0.0f, 0.0f};
			float Health = 100.0f; /* @todo: Some DEFAULT variable to use for places like this instead of hardcoding */
			float MaxHealth = 100.0f;
			std::unordered_set<std::string> TriggeredIDs;
			bool bHasCheckpoint = false;
		};
		FState State;

		std::vector<std::shared_ptr<IItem>> InventorySnapshot;
	};

}
