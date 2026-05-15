#include "checkpointsystem.h"

#include <fstream>
#include <sstream>

#include "core/log.h"
#include "gameplaysystem.h"
#include "instance.h"
#include "player.h"
#include "scene/actor.h"
#include "serialization/serialization.h"

namespace platformer2d {

	void CCheckpointSystem::Initialize(CGameInstance& Owner)
	{
		LK_DEBUG_TAG("CheckpointSystem", "Initialize");
		OwnerRef = &Owner;
	}

	void CCheckpointSystem::Shutdown()
	{
		LK_DEBUG_TAG("CheckpointSystem", "Shutdown");
		OwnerRef = nullptr;
	}

	bool CCheckpointSystem::TrySave(CPlayer& Player, std::string_view CheckpointID, const std::filesystem::path& ScenePath)
	{
		if (CheckpointID.empty()) {
			LK_ERROR_TAG("Checkpoint", "Empty checkpoint ID");
			return false;
		}

		const std::string IDStr(CheckpointID);
		if (State.TriggeredIDs.contains(IDStr)) {
			LK_TRACE_TAG("Checkpoint", "[{}] already triggered, skipping", IDStr);
			return false;
		}

		const FHealthComponent* HC = Player.TryGetComponent<FHealthComponent>();
		const glm::vec3 Pos = Player.GetPosition();
		State.ScenePath = ScenePath;
		State.CurrentID = IDStr;
		State.Position = glm::vec2(Pos.x, Pos.y);
		State.Health = (HC ? HC->GetHealth() : 100.0f); /* @todo: Need to define something like PLAYER_DEFAULT_HEALTH */
		State.MaxHealth = (HC ? HC->GetMaxHealth() : 100.0f);
		State.TriggeredIDs.insert(IDStr);
		State.bHasCheckpoint = true;

		InventorySnapshot = Player.GetInventory().Snapshot();
		LK_INFO_TAG("Checkpoint", R"(Saved "{}" at {} HP={}/{} Inv={})", IDStr, State.Position, State.Health, State.MaxHealth, InventorySnapshot.size());

		SaveToDisk();
		OnSaved.Broadcast(State.CurrentID);

		return true;
	}

	bool CCheckpointSystem::RestoreToPlayer(CPlayer& Player)
	{
		if (!State.bHasCheckpoint) {
			LK_DEBUG_TAG("Checkpoint", "No checkpoint to restore");
			return false;
		}

		LK_INFO_TAG("Checkpoint", R"(Restoring "{}" -> {} HP={}/{})", State.CurrentID, State.Position, State.Health, State.MaxHealth);
		LK_ASSERT(OwnerRef);
		OwnerRef->GetSystem<CGameplaySystem>().Teleport(&Player, State.Position);

		if (FHealthComponent* HC = Player.TryGetComponent<FHealthComponent>()) {
			HC->SetMaxHealth(State.MaxHealth);
			HC->SetHealth(State.Health);
		}

		if (CBody* Body = Player.GetBody()) {
			Body->SetEnabled(true);
			Body->SetLinearVelocity({0.0f, 0.0f});
			Body->SetAwake(true);
		}

		if (!InventorySnapshot.empty()) {
			Player.GetInventory().Restore(InventorySnapshot);
		}

		return true;
	}

	bool CCheckpointSystem::HasCheckpoint() const
	{
		return State.bHasCheckpoint;
	}

	bool CCheckpointSystem::LoadFromDisk(const std::filesystem::path& LevelFilepath)
	{
		State.LevelFilepath = LevelFilepath;
		const std::filesystem::path Path = DeriveCheckpointPath(LevelFilepath);
		if (!std::filesystem::exists(Path)) {
			LK_WARN_TAG("Checkpoint", R"(Checkpoint file does not exist: "{}")", Path);
			return false;
		}

		std::ifstream File(Path);
		if (!File.is_open()) {
			LK_ERROR_TAG("Checkpoint", R"(Failed to open: "{}")", Path);
			return false;
		}

		std::stringstream Buf;
		Buf << File.rdbuf();
		const YAML::Node Node = YAML::Load(Buf.str());
		Serialization::DeserializeProperty("Scene", State.ScenePath, State.ScenePath, Node);
		Serialization::DeserializeProperty("CurrentID", State.CurrentID, State.CurrentID, Node);
		Serialization::DeserializeProperty("Position", State.Position, State.Position, Node);
		Serialization::DeserializeProperty("Health", State.Health, State.Health, Node);
		Serialization::DeserializeProperty("MaxHealth", State.MaxHealth, State.MaxHealth, Node);

		State.TriggeredIDs.clear();
		if (const YAML::Node IDs = Node["TriggeredIDs"]; IDs && IDs.IsSequence()) {
			for (std::size_t Idx = 0; Idx < IDs.size(); Idx++) {
				State.TriggeredIDs.insert(IDs[Idx].as<std::string>());
			}
		}

		State.bHasCheckpoint = !State.CurrentID.empty();
		LK_INFO_TAG("Checkpoint", R"(Loaded "{}" from {} ({} triggered IDs))", State.CurrentID, Path, State.TriggeredIDs.size());

		return State.bHasCheckpoint;
	}

	bool CCheckpointSystem::SaveToDisk()
	{
		if (State.LevelFilepath.empty()) {
			LK_ERROR_TAG("Checkpoint", "Cannot save, level filepath is empty");
			return false;
		}

		LK_DEBUG_TAG("Checkpoint", R"(LevelFilepath: "{}")", State.LevelFilepath);
		const std::filesystem::path Path = DeriveCheckpointPath(State.LevelFilepath);
		LK_INFO_TAG("Checkpoint", "Save to {}", Path);

		YAML::Emitter Out;
		Out << YAML::BeginMap;
		Out << YAML::Key << "Scene" << YAML::Value << State.ScenePath.string();
		Out << YAML::Key << "CurrentID" << YAML::Value << State.CurrentID;
		Out << YAML::Key << "Position" << YAML::Value << State.Position;
		Out << YAML::Key << "Health" << YAML::Value << State.Health;
		Out << YAML::Key << "MaxHealth" << YAML::Value << State.MaxHealth;

		Out << YAML::Key << "TriggeredIDs" << YAML::Value << YAML::BeginSeq;
		for (const std::string& ID : State.TriggeredIDs) {
			Out << ID;
		}
		Out << YAML::EndSeq;

		Out << YAML::EndMap;

		std::ofstream File(Path);
		if (!File.is_open()) {
			LK_ERROR_TAG("Checkpoint", R"(Failed to open: "{}")", Path);
			return false;
		}

		File << Out.c_str();
		return true;
	}

	void CCheckpointSystem::Clear()
	{
		LK_DEBUG_TAG("Checkpoint", "Clear");
		State = FState{};
		InventorySnapshot.clear();
	}

	std::string_view CCheckpointSystem::GetCurrentID() const
	{
		return State.CurrentID;
	}

	std::filesystem::path CCheckpointSystem::DeriveCheckpointPath(const std::filesystem::path& LevelFilepath)
	{
		std::string Out = LevelFilepath.string();
		Out = Out.substr(0, Out.find(".yaml"));
		Out += "_checkpoint.yaml";
		return Out;
	}

	FCheckpointPreview CCheckpointSystem::PeekFromDisk(const std::filesystem::path& LevelFilepath)
	{
		FCheckpointPreview Preview;
		const std::filesystem::path Path = DeriveCheckpointPath(LevelFilepath);
		if (!std::filesystem::exists(Path)) {
			return Preview;
		}

		std::ifstream File(Path);
		if (!File.is_open()) {
			return Preview;
		}

		std::stringstream Buf;
		Buf << File.rdbuf();
		const YAML::Node Node = YAML::Load(Buf.str());

		Serialization::DeserializeProperty("CurrentID", Preview.CurrentID, std::string{}, Node);
		if (const YAML::Node IDs = Node["TriggeredIDs"]; IDs && IDs.IsSequence()) {
			Preview.TriggeredCount = IDs.size();
		}
		Preview.bExists = !Preview.CurrentID.empty();
		return Preview;
	}

}

