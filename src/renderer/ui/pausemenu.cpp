#include "pausemenu.h"

#include "core/core.h"
#include "core/settings.h"
#include "core/window.h"
#include "game/instance.h"
#include "renderer/color.h"
#include "renderer/renderer.h"
#include "renderer/ui/settingsmenu.h"
#include "renderer/ui/ui.h"
#include "renderer/ui/ui_core.h"
#include "renderer/ui/uilayer.h"
#include "renderer/ui/widgets.h"

namespace platformer2d::UI {

	FPauseMenu PauseMenu{};
	FOnPauseMenuOpened OnPauseMenuOpened;

	static void DrawPauseMenu_Default();
	static void DrawPauseMenu_Settings();

	void OpenPauseMenu(const EPauseMenuView View)
	{
		PauseMenu.View = View;
		PauseMenu.bOpen = true;
		OnPauseMenuOpened.Broadcast(PauseMenu.bOpen);
	}

	void ClosePauseMenu(const EPauseMenuView View)
	{
		PauseMenu.View = View;
		PauseMenu.bOpen = false;
		OnPauseMenuOpened.Broadcast(PauseMenu.bOpen);
	}

	void TogglePauseMenu()
	{
		PauseMenu.bOpen = !PauseMenu.bOpen;
		LK_TRACE_TAG("UI", "Toggle Pause Menu: {}", PauseMenu.bOpen ? "Open" : "Closed");
		OnPauseMenuOpened.Broadcast(PauseMenu.bOpen);
	}

	bool IsPauseMenuOpen()
	{
		return PauseMenu.bOpen;
	}

	void DrawMenuTitle(const ImVec2& MenuSize)
	{
		ImGui::SetCursorPosY(16.0f);
		UI::Font::Push(EFont::Roboto, EFontSize::Banner, EFontModifier::BoldItalic);
		static const std::string Title = "platformer2d";
		const ImVec2 TitleSize = ImGui::CalcTextSize(Title.c_str());
		ImGui::SetCursorPosX((MenuSize.x * 0.50f) - (TitleSize.x * 0.50f));
		UI::Text::ColdGradient("platformer2d");
		UI::Font::Pop();

		UI::Font::Push(EFont::Roboto, EFontSize::Regular, EFontModifier::BoldItalic);
		static const std::string Desc = "made by lukkelele";
		const ImVec2 DescSize = ImGui::CalcTextSize(Desc.c_str());
		ImGui::SetCursorPosX((MenuSize.x * 0.50f) - (DescSize.x * 0.50f));
		ImGui::TextColored(ImColor(IM_COL32(100, 100, 100, 255)), Desc.c_str());
		UI::HoverText("Lukas Gunnarsson");
		UI::Font::Pop();

		ImGui::Dummy(ImVec2(0, 52));
	}

	void DrawPauseMenuBackdrop()
	{
		ImGuiViewport* Viewport = ImGui::GetMainViewport();
		if (!Viewport) {
			return;
		}

		constexpr ImGuiWindowFlags BackdropFlags = ImGuiWindowFlags_NoDecoration
			| ImGuiWindowFlags_NoDocking
			| ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoInputs
			| ImGuiWindowFlags_NoNavFocus
			| ImGuiWindowFlags_NoBringToFrontOnFocus
			| ImGuiWindowFlags_NoFocusOnAppearing
			| ImGuiWindowFlags_NoSavedSettings;

		ImGui::SetNextWindowPos(Viewport->Pos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(Viewport->Size, ImGuiCond_Always);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(25, 25, 25, 229));
		if (ImGui::Begin("##PauseMenuBackdrop", nullptr, BackdropFlags)) {
			ImGui::End();
		}
		ImGui::PopStyleColor(1);
		ImGui::PopStyleVar(3);
	}

	void DrawPauseMenu()
	{
		if (!IsPauseMenuOpen()) {
			return;
		}

		DrawPauseMenuBackdrop();

		ImGuiViewport* Viewport = ImGui::GetMainViewport();
		if (!Viewport) {
			return;
		}

		constexpr float Y_FACTOR = 0.85f;
		const bool WideView = (PauseMenu.View == EPauseMenuView::Settings);
		const float WidthMin = WideView ? 900.0f : 630.0f;
		const float WidthMax = WideView ? 1140.0f : 680.0f;
		const float WidthFactor = WideView ? 0.90f : 0.33f;
		const ImVec2 WindowSize = ImVec2((std::clamp(Viewport->Size.x * WidthFactor, WidthMin, WidthMax)),
			(Viewport->Size.y * Y_FACTOR));
		const ImVec2 WindowPos = ImVec2((Viewport->Size.x * 0.50f) - (WindowSize.x * 0.50f),
			((Viewport->Size.y * (1.0f - Y_FACTOR)) * 0.50f));

		ImGui::SetNextWindowPos(WindowPos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(WindowSize, ImGuiCond_Always);
		constexpr ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoDecoration
			| ImGuiWindowFlags_NoDocking
			| ImGuiWindowFlags_NoCollapse;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 0.0f));
		const bool PauseMenuWindowOpened = UI::Begin("##PauseMenu", nullptr, WindowFlags);
		ImGui::PopStyleVar(2);
		if (!PauseMenuWindowOpened) {
			return;
		}

		switch (PauseMenu.View) {
			case EPauseMenuView::Default:
				DrawPauseMenu_Default();
				break;
			case EPauseMenuView::Settings:
				DrawPauseMenu_Settings();
				break;
		}

		if (PauseMenu.View != PauseMenu.LastView) {
			LK_TRACE_TAG("UILayer", "View changed");
		}

		PauseMenu.LastView = PauseMenu.View;

		UI::End();

		UI::DrawSettingsDebugTools(PauseMenu.Settings);
	}

	void DrawPauseMenu_Default()
	{
		const ImVec2 MenuSize = ImGui::GetContentRegionAvail();
		ImGuiStyle& Style = ImGui::GetStyle();

		/* Menu title. */
		DrawMenuTitle(MenuSize);

		/* @todo: Use a table for all the menu options */

		static constexpr float OptionPercentage = 0.80f;
		const ImVec2 ButtonSize = {MenuSize.x * OptionPercentage, 62.0f};

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 14.0f);
		UI::Font::Push(EFont::Roboto, EFontSize::Header, EFontModifier::Bold);

		ImGui::SetCursorPosX(((1.0f - OptionPercentage) * 0.50f) * MenuSize.x);
		if (ImGui::Button(LK_ICON_BOOK " Main Menu", ButtonSize)) {
			Core::Global.RemoveAllLayers();
			CUILayer::RequestMenu(CUILayer::EMenu::MainMenu);
		}

		ImGui::SetCursorPosX(((1.0f - OptionPercentage) * 0.50f) * MenuSize.x);
		if (ImGui::Button(LK_ICON_COG " Settings", ButtonSize)) {
			PauseMenu.View = EPauseMenuView::Settings;
		}

		ImGui::PopStyleVar(1); /* FrameRounding */

		/* Place Quit and Play buttons at the bottom. */
		ImGui::SetCursorPosY(MenuSize.y - ButtonSize.y - 2 * (Style.ItemSpacing.y + Style.FramePadding.y));

		const ImVec2 HalfButtonSize = {(ButtonSize.x * 0.50f), ButtonSize.y};

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
		/* Quit button. */
		{
			UI::FScopedColorStack ColorStack(
				ImGuiCol_Button, IM_COL32(255, 45, 45, 200),
				ImGuiCol_ButtonHovered, IM_COL32(255, 45, 45, 90));
			ImGui::SetCursorPosX(((1.0f - OptionPercentage) * 0.50f) * MenuSize.x);
			if (ImGui::Button("Quit Game", HalfButtonSize)) {
				Core::Global.bShouldShutdown = true;
			}
		}

		/* Play button. */
		ImGui::SameLine();
		{
			UI::FScopedColorStack ColorStack(
				ImGuiCol_Button, RGBA32::NiceGreen,
				ImGuiCol_ButtonHovered, IM_COL32(0, 205, 15, 90));
			if (ImGui::Button(LK_ICON_PLAY " Play", HalfButtonSize)) {
				UI::ClosePauseMenu();
			}
		}
		ImGui::PopStyleVar(1); /* FrameRounding */

		UI::Font::Pop();
	}

	void DrawPauseMenu_Settings()
	{
		const ImVec2 MenuSize = ImGui::GetContentRegionAvail();
		constexpr float BUTTON_HEIGHT = 62.0f;
		constexpr float SAVE_BUTTON_WIDTH = 200.0f;
		constexpr float SPACING = 8.0f;

		const float ContentHeight = MenuSize.y - BUTTON_HEIGHT - SPACING;
		ImGui::BeginChild("##PauseSettingsBody", ImVec2(MenuSize.x, ContentHeight),
			ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		UI::DrawSettingsPanel(PauseMenu.Settings);
		ImGui::EndChild();

		const float BackWidth = std::max(120.0f, MenuSize.x - SAVE_BUTTON_WIDTH - SPACING);
		{
			UI::FScopedColor ButtonHovered(ImGuiCol_ButtonHovered, RGBA32::Compliment);
			if (ImGui::Button(LK_ICON_BACKWARD, ImVec2(BackWidth, BUTTON_HEIGHT))) {
				PauseMenu.View = EPauseMenuView::Default;
			}
		}
		ImGui::SameLine(0.0f, SPACING);
		UI::DrawSaveButton(PauseMenu.Settings, ImVec2(SAVE_BUTTON_WIDTH, BUTTON_HEIGHT));
	}

}

