#pragma once

#include "core/core.h"

namespace platformer2d {

	enum class EGameMenuView
	{
		Default,
		Settings,
	};

	struct FGameMenu
	{
		bool bOpen = false;

		EGameMenuView View = EGameMenuView::Default;
		EGameMenuView LastView = EGameMenuView::Default;

		struct FSettings
		{
			bool bDebug = false;
			bool bStyleEditor = false;
			bool bIDStackTool = false;
		} Settings;
	};

}
