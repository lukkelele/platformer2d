#pragma once

#include <imgui/imgui.h>

#include "core/core.h"
#include "core/delegate.h"

namespace platformer2d::UI {

	enum class EPauseMenuView : std::uint8_t
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

	LK_DECLARE_MULTICAST_DELEGATE(FOnPauseMenuOpened, bool);
	extern FOnPauseMenuOpened OnPauseMenuOpened;

	void OpenPauseMenu(EPauseMenuView View = EPauseMenuView::Default);
	void ClosePauseMenu(EPauseMenuView View = EPauseMenuView::Default);
	void TogglePauseMenu();
	bool IsPauseMenuOpen();

	void DrawPauseMenuBackdrop();
	void DrawPauseMenu();
	void DrawMenuTitle(const ImVec2& MenuSize);

}
