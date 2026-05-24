#pragma once

#include <memory>

#include "core/uuid.h"

namespace platformer2d {
	class CScene;
}

namespace platformer2d::UI {

	void EnemyTools(const std::shared_ptr<CScene>& Scene);
	void RenderEnemySpawnPoints(const std::shared_ptr<CScene>& Scene);

	[[nodiscard]] bool IsSpawnPointGloballyVisible();
	void SetSpawnPointGloballyVisible(bool Enabled);

	[[nodiscard]] bool IsSpawnPointVisible(LUUID Handle);
	void SetSpawnPointVisible(LUUID Handle, bool Visible);

}

