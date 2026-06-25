#include "enemyspawner.h"

#include <cstdlib>

#include "enemy.h"
#include "spawner.h"

namespace platformer2d {

	CEnemySpawner::CEnemySpawner(const FActorSpecification& InSpec)
		: CActor(InSpec)
	{
		LK_DEBUG_TAG("EnemySpawner", "Create: {}", InSpec.Name);
	}

	void CEnemySpawner::Tick(const float DeltaTime)
	{
		CActor::Tick(DeltaTime);

		if (DeltaTime <= 0.0f) {
			return;
		}

		if (!bActive) {
			if (bAutoActivate && (CurrentWave == 0) && !bWaveStarted) {
				Activate();
			}
			if (!bActive) {
				return;
			}
		}

		if (CurrentWave >= Waves.size()) {
			if (bLoop) {
				Reset();
				bActive = true;
			} else {
				bActive = false;
			}
			return;
		}

		const FSpawnWave& Wave = Waves.at(CurrentWave);

		if (!bWaveStarted) {
			Timer += DeltaTime;
			if (Timer < Wave.StartDelay) {
				return;
			}
			BeginWave();
		}

		if (SpawnedInWave < Wave.Count) {
			Timer += DeltaTime;
			if ((Timer >= Wave.SpawnInterval) && (CountAlive() < static_cast<std::size_t>(MaxAlive))) {
				Timer = 0.0f;
				SpawnOne(Wave);
				SpawnedInWave++;
			}
			return;
		}

		if (Wave.bWaitForClear && (CountAlive() > 0)) {
			return;
		}

		CurrentWave++;
		bWaveStarted = false;
		Timer = 0.0f;
	}

	void CEnemySpawner::Activate()
	{
		if (bActive) {
			return;
		}
		LK_DEBUG_TAG("EnemySpawner", "[{}] Activate ({} waves)", GetName(), Waves.size());
		bActive = true;
	}

	void CEnemySpawner::Deactivate()
	{
		bActive = false;
	}

	void CEnemySpawner::Reset()
	{
		bActive = false;
		CurrentWave = 0;
		SpawnedInWave = 0;
		Timer = 0.0f;
		bWaveStarted = false;
		Spawned.clear();
	}

	void CEnemySpawner::SetWaves(std::vector<FSpawnWave> InWaves)
	{
		Waves = std::move(InWaves);
	}

	void CEnemySpawner::BeginWave()
	{
		bWaveStarted = true;
		SpawnedInWave = 0;
		const FSpawnWave& Wave = Waves.at(CurrentWave);
		Timer = Wave.SpawnInterval;
		LK_DEBUG_TAG("EnemySpawner", "[{}] Begin wave {} ({} x {})", GetName(), CurrentWave, Wave.Count, Enum::ToString(Wave.Archetype));
	}

	std::size_t CEnemySpawner::CountAlive()
	{
		std::erase_if(Spawned, [](const std::weak_ptr<CEnemy>& Ref)
		{
			return Ref.expired();
		});

		std::size_t Count = 0;
		for (const std::weak_ptr<CEnemy>& Ref : Spawned) {
			if (const std::shared_ptr<CEnemy> Enemy = Ref.lock(); Enemy && !Enemy->IsDead()) {
				Count++;
			}
		}
		return Count;
	}

	void CEnemySpawner::SpawnOne(const FSpawnWave& Wave)
	{
		const glm::vec3 Origin = GetPosition();
		const float Normalized = (2.0f * (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX))) - 1.0f;
		const glm::vec2 Position = {Origin.x + (Normalized * Scatter), Origin.y};

		if (std::shared_ptr<CEnemy> Enemy = CSpawner::CreateEnemy(Wave.Archetype, Position)) {
			Spawned.push_back(Enemy);
		}
	}

	bool CEnemySpawner::Serialize(YAML::Emitter& Out, EExtendableSerializer Extendable) const
	{
		CActor::Serialize(Out, EExtendableSerializer::Yes);

		Out << YAML::Key << "Spawner";
		Out << YAML::BeginMap;
		Out << YAML::Key << "MaxAlive" << YAML::Value << MaxAlive;
		Out << YAML::Key << "Loop" << YAML::Value << bLoop;
		Out << YAML::Key << "Scatter" << YAML::Value << Scatter;
		Out << YAML::Key << "AutoActivate" << YAML::Value << bAutoActivate;
		Out << YAML::Key << "Waves" << YAML::Value;
		Out << YAML::BeginSeq;
		for (const FSpawnWave& Wave : Waves) {
			Out << YAML::BeginMap;
			Out << YAML::Key << "Archetype" << YAML::Value << static_cast<std::size_t>(Wave.Archetype);
			Out << YAML::Key << "Count" << YAML::Value << Wave.Count;
			Out << YAML::Key << "SpawnInterval" << YAML::Value << Wave.SpawnInterval;
			Out << YAML::Key << "StartDelay" << YAML::Value << Wave.StartDelay;
			Out << YAML::Key << "WaitForClear" << YAML::Value << Wave.bWaitForClear;
			Out << YAML::EndMap;
		}
		Out << YAML::EndSeq;
		Out << YAML::EndMap; /* ~Spawner */

		Out << YAML::EndMap; /* ~Actor */
		return true;
	}

}
