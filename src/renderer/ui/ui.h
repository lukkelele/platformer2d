#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/core.h"
#include "core/delegate.h"
#include "core/math/math.h"
#include "renderer/font.h"
#include "renderer/ui/scoped.h"
#include "scene/actor.h"

namespace platformer2d::UI {

	LK_DECLARE_MULTICAST_DELEGATE(FOnGameMenuOpened, bool);
	extern FOnGameMenuOpened OnGameMenuOpened;

	void Initialize();
	void Render();

	void OpenGameMenu();
	void CloseGameMenu();
	void ToggleGameMenu();
	bool IsGameMenuOpen();

	/**
	 * @brief Combo dropdown.
	 */
	bool BlendFunction();

	void DrawGizmo(int Operation, CActor& Actor, const glm::mat4& ViewMatrix,
				   const glm::mat4& ProjectionMatrix, const glm::vec3& CameraPos = glm::vec3(0.0f, 0.0f, 0.0f));

}