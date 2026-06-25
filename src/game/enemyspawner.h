#pragma once

#include <vector>

#include "game/enemyarchetype.h"
#include "scene/actor.h"

namespace platformer2d {

	class CEnemy;

	struct FSpawnWave
	{
		EEnemyArchetype Archetype = EEnemyArchetype::Grunt;
		int Count = 3;
		float SpawnInterval = 0.50f;
		float StartDelay = 0.0f;
		bool bWaitForClear = true;
	};

	class CEnemySpawner : public CActor
	{
	public:
		CEnemySpawner(const FActorSpecification& InSpec);
		CEnemySpawner(CEnemySpawner&&) = delete;
		CEnemySpawner(const CEnemySpawner&) = delete;
		virtual ~CEnemySpawner() = default;

		void Tick(float DeltaTime) override;
		[[nodiscard]] EActorType GetActorType() const override { return EActorType::Spawner; }

		void Activate();
		void Deactivate();
		void Reset();
		[[nodiscard]] bool IsActive() const { return bActive; }
		[[nodiscard]] bool IsFinished() const { return !bActive && (CurrentWave >= Waves.size()); }

		void SetWaves(std::vector<FSpawnWave> InWaves);
		[[nodiscard]] const std::vector<FSpawnWave>& GetWaves() const { return Waves; }
		[[nodiscard]] std::vector<FSpawnWave>& GetWaves() { return Waves; }

		void SetMaxAlive(const int InMaxAlive) { MaxAlive = InMaxAlive; }
		[[nodiscard]] int GetMaxAlive() const { return MaxAlive; }
		void SetLoop(const bool InLoop) { bLoop = InLoop; }
		[[nodiscard]] bool IsLooping() const { return bLoop; }
		void SetScatter(const float InScatter) { Scatter = InScatter; }
		[[nodiscard]] float GetScatter() const { return Scatter; }
		void SetAutoActivate(const bool InAuto) { bAutoActivate = InAuto; }
		[[nodiscard]] bool IsAutoActivate() const { return bAutoActivate; }

		bool Serialize(YAML::Emitter& Out, EExtendableSerializer Extendable = EExtendableSerializer::No) const override;

	private:
		[[nodiscard]] std::size_t CountAlive();
		void SpawnOne(const FSpawnWave& Wave);
		void BeginWave();

	private:
		std::vector<FSpawnWave> Waves;
		std::vector<std::weak_ptr<CEnemy>> Spawned;

		int MaxAlive = 8;
		bool bLoop = false;
		float Scatter = 0.25f;
		bool bAutoActivate = false;

		bool bActive = false;
		std::size_t CurrentWave = 0;
		int SpawnedInWave = 0;
		float Timer = 0.0f;
		bool bWaveStarted = false;

		LK_CLASS();
	};

}
