#include "ui.h"

#include "core/input/keyboard.h"

namespace platformer2d::UI {

	FOnGameMenuOpened OnGameMenuOpened;

	enum class EGameMenuView
	{
		Default,
		Settings,
	};

	struct FInternalData
	{
		bool bPauseMenu = false;

		EGameMenuView View = EGameMenuView::Default;
		EGameMenuView LastView = EGameMenuView::Default;

		struct FSettings
		{
			bool bDebug = false;
			bool bStyleEditor = false;
			bool bIDStackTool = false;
		} Settings;
	};

	namespace
	{
		FInternalData Internal{};
	}

	FORCEINLINE static bool IsGameMenuOpen() { return Internal.bPauseMenu; }

	FORCEINLINE static void UI_GameMenu_Default()
	{
		const ImVec2 StartCursorPos = ImGui::GetCursorPos();
		const ImVec2 MenuSize = ImGui::GetWindowSize();
		const ImVec2 ButtonSize = { MenuSize.x, 62.0f };
		ImGuiStyle& Style = ImGui::GetStyle();

		if (ImGui::Button("Settings", ButtonSize))
		{
			Internal.View = EGameMenuView::Settings;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
		{
			const ImVec2 Avail = ImGui::GetContentRegionAvail();
			ImGui::SetCursorPosY(Avail.y + Style.ItemSpacing.y);
		}
		if (ImGui::Button("Exit", ButtonSize))
		{
		}
		ImGui::PopStyleVar(1);
	}

	FORCEINLINE static void UI_GameMenu_Settings()
	{
		FInternalData::FSettings& Settings = Internal.Settings;
		ImGuiStyle& Style = ImGui::GetStyle();

		const ImVec2 StartCursorPos = ImGui::GetCursorPos();
		const ImVec2 MenuSize = ImGui::GetWindowSize();
		const ImVec2 ButtonSize = { MenuSize.x, 62.0f };

		static constexpr float PaddingX = 12.0f;
		static constexpr float PaddingY = 12.0f;
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
		ImGui::SetCursorPos(ImVec2(PaddingX, PaddingY));
		ImGui::Checkbox("Debug", &Internal.Settings.bDebug);
		ImGui::PopStyleVar(2);

		ImGui::Dummy(ImVec2(0.0f, PaddingY * 0.50f));
		if (ImGui::Button("Style Editor", ButtonSize))
		{
			Settings.bStyleEditor = !Settings.bStyleEditor;
		}
		if (Settings.bStyleEditor)
		{
			ImGui::Begin("##StyleEditor", &Settings.bStyleEditor);
			ImGuiStyle& Style = ImGui::GetStyle();
			ImGui::ShowStyleEditor(&Style);
			ImGui::End();
		}

		if (ImGui::Button("ID Tool", ButtonSize))
		{
			Settings.bIDStackTool = !Settings.bIDStackTool;
		}
		if (Settings.bIDStackTool)
		{
			ImGui::ShowIDStackToolWindow(&Settings.bIDStackTool);
		}

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
		{
			const ImVec2 Avail = ImGui::GetContentRegionAvail();
			ImGui::SetCursorPosY(Avail.y + ButtonSize.y);
		}
		if (ImGui::Button("<-", ButtonSize))
		{
			Internal.View = EGameMenuView::Default;
		}
		ImGui::PopStyleVar(1);
	}

	FORCEINLINE static void UI_GameMenu()
	{
		ImGuiViewport* Viewport = ImGui::GetMainViewport();
		if (Viewport == nullptr)
		{
			return;
		}

		static constexpr float YFactor = 0.80f;
		const ImVec2 WindowSize((Viewport->Size.x * 0.33f), Viewport->Size.y * YFactor);
		const ImVec2 WindowPos(
			(Viewport->Size.x * 0.50f) - (WindowSize.x * 0.50f),
			(Viewport->Size.y * (1.0f - YFactor)) * 0.50f
		);

		ImGui::SetNextWindowPos(WindowPos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(WindowSize, ImGuiCond_Always);
		static constexpr int WindowFlags = ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoDecoration 
			| ImGuiWindowFlags_NoDocking
			| ImGuiWindowFlags_NoCollapse;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 0.0f));
		if (!ImGui::Begin("##GameMenu", nullptr, WindowFlags))
		{
			return;
		}

		ImGui::PopStyleVar(2);
		const ImVec2 StartCursorPos = ImGui::GetCursorPos();
		const ImVec2 MenuSize = ImGui::GetWindowSize();
		const ImVec2 ButtonSize = { MenuSize.x, 62.0f };

		switch (Internal.View)
		{
			case EGameMenuView::Default:
				UI_GameMenu_Default();
				break;
			case EGameMenuView::Settings:
				UI_GameMenu_Settings();
				break;
		}

		if (Internal.View != Internal.LastView)
		{
			LK_DEBUG_TAG("UI", "View changed");
		}

		Internal.LastView = Internal.View;

		ImGui::End();
	}

	static void OnKeyPressed(const FKeyData& KeyData)
	{
		if (KeyData.State == EKeyState::Pressed)
		{
			switch (KeyData.Key)
			{
				case EKey::Escape:
					ToggleGameMenu();
					break;
			}
		}
	}

	void Initialize()
	{
		CKeyboard::OnKeyPressed.Add(OnKeyPressed);
	}

	void Render()
	{
		if (IsGameMenuOpen())
		{
			UI_GameMenu();
		}
	}

	void OpenGameMenu()
	{
		Internal.bPauseMenu = true;
		OnGameMenuOpened.Broadcast(Internal.bPauseMenu);
	}

	void CloseGameMenu()
	{
		Internal.bPauseMenu = false;
		OnGameMenuOpened.Broadcast(Internal.bPauseMenu);
	}

	void ToggleGameMenu()
	{
		Internal.bPauseMenu = !Internal.bPauseMenu;
		LK_DEBUG_TAG("UI", "Toggle Game Menu: {}", Internal.bPauseMenu ? "Open" : "Closed");
		OnGameMenuOpened.Broadcast(Internal.bPauseMenu);
	}

}
