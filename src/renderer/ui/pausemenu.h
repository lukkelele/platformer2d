#pragma once

#include "core/core.h"

namespace platformer2d::UI {

	enum class EPauseMenuView
	{
		Default,
		Settings,
	};

	struct FPauseMenu
	{
		bool bOpen = false;

		EPauseMenuView View = EPauseMenuView::Default;
		EPauseMenuView LastView = EPauseMenuView::Default;

		struct FSettings
		{
			bool bDebug = false;
			bool bStyleEditor = false;
			bool bIDStackTool = false;
		} Settings;
	};

	extern FPauseMenu PauseMenu;

}
