#include "pausemenu.h"

#include "core/core.h"
#include "core/settings.h"
#include "core/window.h"
#include "game/instance.h"
#include "renderer/color.h"
#include "renderer/renderer.h"
#include "renderer/ui/ui.h"
#include "renderer/ui/ui_core.h"
#include "renderer/ui/uilayer.h"
#include "renderer/ui/widgets.h"

namespace platformer2d::UI {

	FPauseMenu PauseMenu{};
	FOnPauseMenuOpened OnPauseMenuOpened;

	static constexpr float LABEL_COLUMN_WIDTH = 190.0f;
	static constexpr float LABEL_INDENT_WIDTH = 24.0f;
	static constexpr float COLUMN_ITEM_WIDTH = 410.0f;

	static void DrawPauseMenu_Default();
	static void DrawPauseMenu_Settings();
	static void DrawDebugTools();

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
		UI::ColdTextGradient("platformer2d");
		UI::Font::Pop();

		UI::Font::Push(EFont::Roboto, EFontSize::Regular, EFontModifier::BoldItalic);
		static const std::string Desc = "made by lukkelele";
		const ImVec2 DescSize = ImGui::CalcTextSize(Desc.c_str());
		ImGui::SetCursorPosX((MenuSize.x * 0.50f) - (DescSize.x * 0.50f));
		ImGui::TextColored(ImColor(IM_COL32(100, 100, 100, 255)), Desc.c_str());
		UI::HoverText("Lukas Gunnarsson");
		UI::Font::Pop();

		/* @todo Versioning info here */

		ImGui::Dummy(ImVec2(0.0f, 52.0f));
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

		constexpr float Y_FACTOR = 0.80f;
		const ImVec2 WindowSize = ImVec2((std::clamp(Viewport->Size.x * 0.33f, 630.0f, 680.0f)),
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

		DrawDebugTools();
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
		FPauseMenu::FSettings& PauseSettings = PauseMenu.Settings;
		ImGuiStyle& Style = ImGui::GetStyle();

		const ImVec2 MenuSize = ImGui::GetContentRegionAvail();
		const ImVec2 ButtonSize = {MenuSize.x, 62.0f};

		UI::ShiftCursorY(12.0f);
		UI::BannerTextCentralized("Settings", EFont::Roboto, EFontModifier::Bold);
		ImGui::Dummy(ImVec2(0, 10));

		UI::Font::Push(EFont::Roboto, EFontSize::Larger, EFontModifier::Bold);

		constexpr float PaddingY = 12.0f;
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

		UI::ShiftCursorX(20);
		ImGui::PushStyleColor(ImGuiCol_Text, RGBA32::Text::Darker);
		UI::HeaderText("Window", EFont::Roboto, EFontModifier::Bold);
		ImGui::PopStyleColor(1);
		UI::ShiftCursor(44, 10);
		if (ImGui::BeginTable("##WindowSettings", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip)) {
			FSettings& AppSettings = FSettings::Get();
			const ImVec2 Avail = ImGui::GetContentRegionAvail();
			const float IndentX = Avail.x * 0.35f;
			ImGui::TableSetupColumn("Label", 0, Avail.x * 0.50f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, ImGui::GetContentRegionAvail().x * 0.50f);

			ImGui::TableNextRow();
			if (UI::Checkbox("VSync", AppSettings.Window.bVSync, IndentX)) {
				CWindow::Get().SetVSync(AppSettings.Window.bVSync);
			}

			ImGui::TableNextRow();
			UI::Checkbox("Start Maximized", AppSettings.Window.bStartMaximized, IndentX);

			ImGui::TableNextRow();
			UI::Checkbox("Debug", PauseSettings.bDebug, IndentX);

			ImGui::EndTable();
		}

		UI::ShiftCursorX(20);
		ImGui::PushStyleColor(ImGuiCol_Text, RGBA32::Text::Darker);
		UI::HeaderText("Renderer", EFont::Roboto, EFontModifier::Bold);
		ImGui::PopStyleColor(1);
		UI::ShiftCursor(44, 10);
		if (ImGui::BeginTable("##RenderSettings", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip)) {
			const ImVec2 Avail = ImGui::GetContentRegionAvail();
			const float IndentX = Avail.x * 0.35f;
			ImGui::TableSetupColumn("Label", 0, Avail.x * 0.35f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, ImGui::GetContentRegionAvail().x * 0.50f);

			ImGui::TableNextRow();
			bool bDepthTest = CRenderer::GetDepthTest();

			if (UI::Checkbox("Depth Test", bDepthTest, IndentX)) {
				CRenderer::SetDepthTest(bDepthTest);
			}

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			UI::BlendFunction();

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			UI::DepthFunction();

			ImGui::EndTable();
		}

		ImGui::SeparatorText("Camera");
		if (CGameInstance::IsValid()) {
			auto& GameInstance = CGameInstance::Get();
			if (CCamera* Camera = GameInstance.GetActiveCamera(); Camera != nullptr) {
				ImGui::PushID("UI_CameraOptions");
				ImGui::BeginTable("##CameraOptionsTable", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip);
				ImGui::TableSetupColumn("Label", 0, LABEL_COLUMN_WIDTH);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, ImGui::GetContentRegionAvail().x - LABEL_COLUMN_WIDTH);

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				UI::ShiftCursor(LABEL_INDENT_WIDTH, 4.0f);
				ImGui::Text("Zoom");

				ImGui::TableSetColumnIndex(1);
				UI::ShiftCursor(0.0f, 4.0f);
				ImGui::SetNextItemWidth(COLUMN_ITEM_WIDTH);
				float Zoom = Camera->GetZoom();
				ImGui::SliderFloat("##CameraZoom", &Zoom, CCamera::ZOOM_MIN, CCamera::ZOOM_MAX, "%.2f");
				if (ImGui::IsItemActive()) {
					Camera->SetZoom(Zoom);
				}

				ImGui::EndTable();
				ImGui::PopID();
			}
		}

		ImGui::Dummy(ImVec2(0.0f, 12.0f + PaddingY * 0.50f));
		if (ImGui::Button("Style Editor", ButtonSize)) {
			PauseSettings.bStyleEditor = !PauseSettings.bStyleEditor;
		}
		if (ImGui::Button("ID Tool", ButtonSize)) {
			PauseSettings.bIDStackTool = !PauseSettings.bIDStackTool;
		}

		ImGui::PopStyleVar(2);
		UI::Font::Pop();

		/* Button: Backward */
		{
			UI::ShiftCursorY(ImGui::GetContentRegionAvail().y - ButtonSize.y);
			UI::FScopedColor ButtonHovered(ImGuiCol_ButtonHovered, RGBA32::Compliment);
			if (ImGui::Button(LK_ICON_BACKWARD, ButtonSize)) {
				PauseMenu.View = EPauseMenuView::Default;
			}
		}
	}

	void DrawDebugTools()
	{
		ImGuiStyle& Style = ImGui::GetStyle();
		FPauseMenu::FSettings& PauseSettings = PauseMenu.Settings;
		if (PauseSettings.bStyleEditor) {
			UI::FScopedFont Font(UI::Font::Get(EFont::SourceSansPro, EFontSize::Regular, EFontModifier::Normal));
			if (ImGui::Begin("Style Editor", &PauseSettings.bStyleEditor)) {
				ImGui::ShowStyleEditor(&Style);
				ImGui::End();
			}
		}
		if (PauseSettings.bIDStackTool) {
			ImGui::ShowIDStackToolWindow(&PauseSettings.bIDStackTool);
		}
	}

}

