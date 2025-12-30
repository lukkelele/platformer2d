#include "uilayer.h"

#include "core/window.h"
#include "core/settings.h"
#include "game/instance.h"
#include "renderer/renderer.h"
#include "renderer/ui/ui.h"
#include "renderer/ui/widgets.h"
#include "gamemenu.h"

namespace platformer2d {

	static void UI_GameMenu_Default();
	static void UI_GameMenu_Settings();

	namespace {
		FGameMenu GameMenu{};
		constexpr float LABEL_COLUMN_WIDTH = 190.0f;
		constexpr float LABEL_INDENT_WIDTH = 24.0f;
		constexpr float COLUMN_ITEM_WIDTH = 410.0f;
	}

	CUILayer::CUILayer(std::string_view InName)
		: CLayer(InName)
	{
		ImGuiLayer = std::make_unique<CImGuiLayer>(CWindow::Get()->GetGlfwWindow());
	}

	void CUILayer::OnAttach()
	{
		LK_TRACE_TAG("UILayer", "OnAttach");
		CKeyboard::OnKeyPressed.Add(this, &CUILayer::OnKeyPressed);
	}

	void CUILayer::OnDetach()
	{
		LK_TRACE_TAG("UILayer", "OnDetach");
		if (ImGuiLayer) {
			LK_DEBUG_TAG("UILayer", "Destroy ImGui layer");
			ImGuiLayer->Destroy();
			ImGuiLayer.reset();
			ImGuiLayer = nullptr;
		}
	}

	void CUILayer::Tick(const float DeltaTime)
	{
		LK_UNUSED(DeltaTime);

		/* Draw dark overlay whenever the pause menu is open. */
		if (CGameInstance* GameInstance = CGameInstance::Get()) {
			CWindow* Window = CWindow::Get();
			if (Window && GameInstance->HasScene() && UI::IsGameMenuOpen()) {
				const glm::vec2 WindowSize = Window->GetSize();
				static constexpr glm::vec4 OverlayColor = { 0.10f, 0.10f, 0.10f, 0.90f };
				CRenderer::DrawQuad(glm::vec3(0.0f, 0.0f, 0.0f), WindowSize, OverlayColor);
			}
		}
	}

	void CUILayer::RenderUI()
	{
		if (CGameInstance::Get()) {
			if (UI::IsGameMenuOpen()) {
				UI_GameMenu();
			}
		} else {
			UI::StartMenu();
		}
	}

	void CUILayer::BeginFrame()
	{
		ImGuiLayer->BeginFrame();
	}

	void CUILayer::EndFrame()
	{
		ImGuiLayer->EndFrame();
	}

	void CUILayer::UI_GameMenu()
	{
		ImGuiViewport* Viewport = ImGui::GetMainViewport();
		if (!Viewport) {
			return;
		}

		static constexpr float YFactor = 0.80f;
		const ImVec2 WindowSize = ImVec2(
			(std::clamp(Viewport->Size.x * 0.33f, 630.0f, 680.0f)),
			(Viewport->Size.y * YFactor)
		);
		const ImVec2 WindowPos = ImVec2(
			(Viewport->Size.x * 0.50f) - (WindowSize.x * 0.50f),
			((Viewport->Size.y * (1.0f - YFactor)) * 0.50f)
		);

		ImGui::SetNextWindowPos(WindowPos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(WindowSize, ImGuiCond_Always);
		static constexpr int WindowFlags = ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoDecoration
			| ImGuiWindowFlags_NoDocking
			| ImGuiWindowFlags_NoCollapse;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 0.0f));
		const bool GameMenuWindowOpened = UI::Begin("##GameMenu", nullptr, WindowFlags);
		ImGui::PopStyleVar(2);
		if (!GameMenuWindowOpened) {
			return;
		}

		const ImVec2 StartCursorPos = ImGui::GetCursorPos();
		const ImVec2 MenuSize = ImGui::GetWindowSize();
		const ImVec2 ButtonSize = { MenuSize.x, 62.0f };

		switch (GameMenu.View) {
			case EGameMenuView::Default:
				UI_GameMenu_Default();
				break;
			case EGameMenuView::Settings:
				UI_GameMenu_Settings();
				break;
		}

		if (GameMenu.View != GameMenu.LastView) {
			LK_TRACE_TAG("UILayer", "View changed");
		}

		GameMenu.LastView = GameMenu.View;

		UI::End();
	}

	void UI_GameMenu_Settings()
	{
		FGameMenu::FSettings& Settings = GameMenu.Settings;
		ImGuiStyle& Style = ImGui::GetStyle();
		CWindow* Window = CWindow::Get();

		const ImVec2 StartCursorPos = ImGui::GetCursorPos();
		const ImVec2 MenuSize = ImGui::GetContentRegionAvail();
		const ImVec2 ButtonSize = { MenuSize.x, 62.0f };

		UI::ShiftCursorY(12.0f);
		UI::BannerTextCentralized("Settings", EFont::Roboto, EFontModifier::Bold);
		ImGui::Dummy(ImVec2(0, 10));

		UI::Font::Push(EFont::Roboto, EFontSize::Larger, EFontModifier::Bold);

		static constexpr float PaddingX = 12.0f;
		static constexpr float PaddingY = 12.0f;
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

		UI::ShiftCursorX(20);
		ImGui::PushStyleColor(ImGuiCol_Text, RGBA32::Text::Darker);
		UI::HeaderText("Window", EFont::Roboto, EFontModifier::Bold);
		ImGui::PopStyleColor(1);
		UI::ShiftCursor(44, 10);
		if (ImGui::BeginTable("##WindowSettings", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip)) {
			FSettings& Settings = FSettings::Get();
			const ImVec2 Avail = ImGui::GetContentRegionAvail();
			const float IndentX = Avail.x * 0.35f;
			ImGui::TableSetupColumn("Label", 0, Avail.x * 0.50f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, ImGui::GetContentRegionAvail().x * 0.50f);

			ImGui::TableNextRow();
			if (UI::Checkbox("VSync", Settings.Window.bVSync, IndentX)) {
				Window->SetVSync(Settings.Window.bVSync);
			}

			ImGui::TableNextRow();
			UI::Checkbox("Start Maximized", Settings.Window.bStartMaximized, IndentX);

			ImGui::TableNextRow();
			UI::Checkbox("Debug", GameMenu.Settings.bDebug, IndentX);

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
			UI::Widget::Combo::BlendFunction();

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			UI::Widget::Combo::DepthFunction();

			ImGui::EndTable();
		}

		ImGui::SeparatorText("Camera");
		if (CGameInstance* GameInstance = CGameInstance::Get(); GameInstance != nullptr) {
			if (CCamera* Camera = GameInstance->GetActiveCamera(); Camera != nullptr) {
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

		ImGui::PopStyleVar(2);

		/************************
		 *     Menu Options
		 ************************/
		ImGui::Dummy(ImVec2(0.0f, 12.0f + PaddingY * 0.50f));
		if (ImGui::Button("Style Editor", ButtonSize)) {
			Settings.bStyleEditor = !Settings.bStyleEditor;
		}
		if (Settings.bStyleEditor) {
			UI::FScopedFont Font(UI::Font::Get(EFont::SourceSansPro, EFontSize::Regular, EFontModifier::Normal));
			if (ImGui::Begin("##StyleEditor", &Settings.bStyleEditor)) {
				ImGui::ShowStyleEditor(&Style);
				ImGui::End();
			}
		}

		if (ImGui::Button("ID Tool", ButtonSize)) {
			Settings.bIDStackTool = !Settings.bIDStackTool;
		}
		if (Settings.bIDStackTool) {
			ImGui::ShowIDStackToolWindow(&Settings.bIDStackTool);
		}

		UI::Font::Pop();

		/* Button: Backward */
		{
			UI::ShiftCursorY(ImGui::GetContentRegionAvail().y - ButtonSize.y);
			UI::FScopedColor ButtonHovered(ImGuiCol_ButtonHovered, RGBA32::Compliment);
			if (ImGui::Button(LK_ICON_BACKWARD, ButtonSize)) {
				GameMenu.View = EGameMenuView::Default;
			}
		}
	}

	static void UI_GameMenu_Title(const ImVec2& MenuSize)
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

	void UI_GameMenu_Default()
	{
		const ImVec2 StartCursorPos = ImGui::GetCursorPos();
		const ImVec2 MenuSize = ImGui::GetContentRegionAvail();
		ImGuiStyle& Style = ImGui::GetStyle();

		/* Menu title. */
		UI_GameMenu_Title(MenuSize);

		static constexpr float OptionPercentage = 0.80f;
		const ImVec2 ButtonSize = { MenuSize.x * OptionPercentage, 62.0f };
		ImGui::SetCursorPosX(((1.0f - OptionPercentage) * 0.50f) * MenuSize.x);

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 14.0f);
		UI::Font::Push(EFont::Roboto, EFontSize::Header, EFontModifier::Bold);
		if (ImGui::Button(LK_ICON_COG " Settings", ButtonSize)) {
			GameMenu.View = EGameMenuView::Settings;
		}
		ImGui::PopStyleVar(1); /* FrameRounding */

		/* Place Quit and Play buttons at the bottom. */
		ImGui::SetCursorPosY(MenuSize.y - ButtonSize.y - 2 * (Style.ItemSpacing.y + Style.FramePadding.y));

		const ImVec2 HalfButtonSize = { (ButtonSize.x * 0.50f), ButtonSize.y };

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
		/* Quit button. */
		ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(255, 45, 45, 200));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 45, 45, 90));
		ImGui::SetCursorPosX(((1.0f - OptionPercentage) * 0.50f) * MenuSize.x);
		if (ImGui::Button("Quit Game", HalfButtonSize)) {
			Core::Global.bShouldShutdown = true;
		}
		ImGui::PopStyleColor(2);

		/* Play button. */
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Button, RGBA32::NiceGreen);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 205, 15, 90));
		if (ImGui::Button(LK_ICON_PLAY " Play", HalfButtonSize)) {
			UI::CloseGameMenu();
		}
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar(1);

		UI::Font::Pop();
	}

	void CUILayer::OnKeyPressed(const FKeyData& KeyData)
	{
		if (KeyData.State == EKeyState::Pressed) {
			switch (KeyData.Key) {
				case EKey::Escape:
					UI::ToggleGameMenu();
					break;
			}
		}
	}

	namespace UI {

		void StartMenu()
		{
			ImGuiViewport* Viewport = ImGui::GetMainViewport();
			if (!Viewport) {
				return;
			}

			static constexpr float XFactor = 0.65f;
			static constexpr float YFactor = 0.80f;
			static constexpr float LabelColumnWidth = 180.0f;

			const ImVec2 ViewportSize = ImVec2(
				(std::clamp(Viewport->Size.x * XFactor, 620.0f, 940.0f)),
				(Viewport->Size.y * YFactor)
			);
			const ImVec2 WindowPos = ImVec2(
				(Viewport->Size.x * 0.50f) - (ViewportSize.x * 0.50f),
				((Viewport->Size.y * (1.0f - YFactor)) * 0.50f)
			);

			ImGui::SetNextWindowPos(WindowPos, ImGuiCond_Always);
			ImGui::SetNextWindowSize(ViewportSize, ImGuiCond_Always);
			static constexpr int WindowFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 8);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 24);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 0.0f));
			ImGui::PushStyleColor(ImGuiCol_WindowBg, RGBA32::DarkGray);
			ImGui::PushStyleColor(ImGuiCol_Border, RGBA32::BackgroundDark);
			const bool WindowOpened = UI::Begin("##MainMenu", nullptr, WindowFlags);
			ImGui::PopStyleVar(4);
			ImGui::PopStyleColor(2);
			if (!WindowOpened) {
				return;
			}

			const ImVec2 StartCursorPos = ImGui::GetCursorPos();
			const ImVec2 WindowSize = ImGui::GetWindowSize();
			const ImVec2 ButtonSize = { WindowSize.x * 0.40f, 72.0f };
			UI_GameMenu_Title(WindowSize);

			auto NextButtonEntry = []() -> void
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
			};

			UI::Font::Push(EFont::Roboto, EFontSize::Header, EFontModifier::Bold);
			if (ImGui::BeginTable("##Menu", 1, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip)) {
				ImGui::TableSetupColumn("L", 0, ImGui::GetContentRegionAvail().x);

				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);

				NextButtonEntry();
				{
					UI::FScopedColorStack ColorStack(
						ImGuiCol_Button, RGBA32::SmoothGreen
					);

					UI::ShiftCursorX((ImGui::GetContentRegionAvail().x * 0.50f) - (ButtonSize.x * 0.50f));
					if (ImGui::Button("Levels", ButtonSize)) {
						LK_WARN("TODO");
					}

					ImGui::Dummy(ImVec2(0, 16));
				}

				NextButtonEntry();
				{
					UI::FScopedColorStack ColorStack(
						ImGuiCol_Button, RGBA32::DarkCyan
					);

					UI::ShiftCursorX((ImGui::GetContentRegionAvail().x * 0.50f) - (ButtonSize.x * 0.50f));
					if (ImGui::Button("Editor", ButtonSize)) {
						Core::Global.AddLayer(Core::ELayer::Editor);
					}

					ImGui::Dummy(ImVec2(0, 16));
				}

				ImGui::EndTable();
			}

			/* Button: Quit */
			{
				ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(255, 45, 45, 200));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 45, 45, 90));
				const ImVec2 Avail = ImGui::GetContentRegionAvail();
				UI::ShiftCursor(
					(Avail.x * 0.50f) - (ButtonSize.x * 0.50f),
					(Avail.y - ButtonSize.y - 40.0f)
				);
				if (ImGui::Button("Quit", ButtonSize)) {
					Core::Global.bShouldShutdown = true;
				}
				ImGui::PopStyleColor(2);
			}

			ImGui::PopStyleVar(1); /* FrameRounding */

			UI::Font::Pop();

			UI::End();
		}

		void OpenGameMenu()
		{
			GameMenu.bOpen = true;
			OnGameMenuOpened.Broadcast(GameMenu.bOpen);
		}

		void CloseGameMenu()
		{
			GameMenu.bOpen = false;
			OnGameMenuOpened.Broadcast(GameMenu.bOpen);
		}

		void ToggleGameMenu()
		{
			GameMenu.bOpen = !GameMenu.bOpen;
			LK_TRACE_TAG("UI", "Toggle Game Menu: {}", GameMenu.bOpen ? "Open" : "Closed");
			OnGameMenuOpened.Broadcast(GameMenu.bOpen);
		}

		bool IsGameMenuOpen()
		{
			return GameMenu.bOpen;
		}

	}
}
