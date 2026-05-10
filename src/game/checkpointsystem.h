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

namespace platformer2d {

	class CPlayer;
	class IItem;

	class CCheckpointSystem
	{
	public:
		LK_DECLARE_MULTICAST_DELEGATE(FOnSaved, std::string_view);
		static inline FOnSaved OnSaved;

	public:
		CCheckpointSystem() = delete;
		~CCheckpointSystem() = delete;
		CCheckpointSystem(CCheckpointSystem&&) = delete;
		CCheckpointSystem(const CCheckpointSystem&) = delete;

		CCheckpointSystem& operator=(CCheckpointSystem&&) = delete;
		CCheckpointSystem& operator=(const CCheckpointSystem&) = delete;

		static bool TrySave(CPlayer& Player, std::string_view CheckpointID, const std::filesystem::path& ScenePath);
		static bool RestoreToPlayer(CPlayer& Player);
		static bool HasCheckpoint();

		static bool LoadFromDisk(const std::filesystem::path& LevelFilepath);
		static bool SaveToDisk();
		static void Clear();

		static std::string_view GetCurrentID();

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

		static std::filesystem::path DeriveCheckpointPath(const std::filesystem::path& LevelFilepath);

		static inline FState State;
		static inline std::vector<std::shared_ptr<IItem>> InventorySnapshot;
	};

}
