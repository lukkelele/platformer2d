#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/core.h"
#include "core/delegate.h"

namespace platformer2d::UI {

	LK_DECLARE_MULTICAST_DELEGATE(FOnGameMenuOpened, bool);
	extern FOnGameMenuOpened OnGameMenuOpened;

	void Initialize();
	void Render();

	void OpenGameMenu();
	void CloseGameMenu();
	void ToggleGameMenu();
}