#include "uilayer.h"

#include "core/profiler.h"
#include "core/window.h"
#include "core/settings.h"
#include "game/instance.h"
#include "renderer/renderer.h"
#include "renderer/ui/ui.h"
#include "renderer/ui/widgets.h"
#include "renderer/ui/pausemenu.h"

namespace platformer2d {

	static void UI_PauseMenu_Default();
	static void UI_PauseMenu_Settings();
	static void UI_RenderWindows();

	struct FNextLevel
	{
		bool bSelected = false;
		std::filesystem::path Path{};

		void Clear()
		{
			bSelected = false;
			Path.clear();
		}
	};

	namespace UI {
		FPauseMenu PauseMenu{};
	}

	namespace {
		constexpr float LABEL_COLUMN_WIDTH = 190.0f;
		constexpr float LABEL_INDENT_WIDTH = 24.0f;
		constexpr float COLUMN_ITEM_WIDTH = 410.0f;

		CUILayer::EMenu NextMenu = CUILayer::EMenu::None;
		FNextLevel NextLevel{};
	}

	CUILayer::CUILayer(std::string_view InName)
		: CLayer(InName)
	{
		NextMenu = ActiveMenu;
	}

	void CUILayer::Initialize()
	{
		ImGuiLayer = std::make_unique<CImGuiLayer>(CWindow::Get()->GetGlfwWindow());

		auto& Settings = FSettings::Get();
		switch (Settings.QuickLoad) {
			case EQuickLoad::None:
				break;
			case EQuickLoad::Editor:
				LK_DEBUG_TAG("UILayer", "Quickloading editor");
				Core::Global.AddLayer(Core::ELayer::Editor);
				break;
		}
	}

	void CUILayer::OnAttach()
	{
		LK_VERIFY(ImGuiLayer);
		LK_DEBUG_TAG("UILayer", "OnAttach");
		DelegateHandles.OnKeyPressed = CKeyboard::OnKeyPressed.Add(this, &CUILayer::OnKeyPressed);
	}

	void CUILayer::OnDetach()
	{
		LK_TRACE_TAG("UILayer", "OnDetach");
		CKeyboard::OnKeyPressed.Remove(DelegateHandles.OnKeyPressed);
		if (ImGuiLayer) {
			LK_DEBUG_TAG("UILayer", "Destroy ImGui layer");
			ImGuiLayer->Destroy();
			ImGuiLayer.reset();
		}
	}

	void CUILayer::Tick(const float DeltaTime)
	{
		LK_PROFILE_FUNC();
		CGameInstance* GameInstance = CGameInstance::Get();
		if (NextLevel.bSelected) {
			LK_ASSERT(GameInstance);
			if (GameInstance) {
				LK_DEBUG_TAG("UILayer", "Level selected: {}", NextLevel.Path);
				GameInstance->OpenScene(NextLevel.Path);
				NextLevel.Clear();
			} else {
				LK_ERROR_TAG("UILayer", "No game instance, cannot enter level");
			}
		}

		/* Draw dark overlay whenever the pause menu is open. */
		if (GameInstance) {
			CWindow* Window = CWindow::Get();
			if (Window && GameInstance->HasScene() && UI::IsPauseMenuOpen()) {
				const glm::vec2 WindowSize = Window->GetSize();
				static constexpr glm::vec4 OverlayColor = {0.10f, 0.10f, 0.10f, 0.90f};
				CRenderer::DrawQuad(glm::vec3(0.0f, 0.0f, 0.0f), WindowSize, OverlayColor);
			}
		}
	}

	void CUILayer::RenderUI()
	{
		LK_PROFILE_FUNC();
		const bool MenuHasChanged = (ActiveMenu != NextMenu);
		if (MenuHasChanged) {
			LK_DEBUG_TAG("UILayer", "Menu changed to: {}", Enum::ToString(NextMenu));
			if (NextMenu == EMenu::MainMenu) {
				LK_DEBUG_TAG("UILayer", "Clearing all layers");
				Core::Global.RemoveAllLayers();
			}
		}

		if (CGameInstance* GameInstance = CGameInstance::Get()) {
			if (GameInstance->HasScene()) {
				if (UI::IsPauseMenuOpen()) {
					UI_PauseMenu();
				}
			} else {
				NextMenu = EMenu::None;
			}
		} else {
			switch (ActiveMenu) {
				case EMenu::None:
					NextMenu = EMenu::MainMenu;
					break;
				case EMenu::MainMenu:
					UI_MainMenu();
					break;
				case EMenu::LevelLauncher:
					UI::LevelLauncher();
					break;
			}
		}

		ActiveMenu = NextMenu;

		UI_RenderWindows();
	}

	void CUILayer::BeginFrame()
	{
		LK_PROFILE_FUNC();
		ImGuiLayer->BeginFrame();
	}

	void CUILayer::EndFrame()
	{
		LK_PROFILE_FUNC();
		ImGuiLayer->EndFrame();
	}

	void CUILayer::UI_PauseMenu()
	{
		ImGuiViewport* Viewport = ImGui::GetMainViewport();
		if (!Viewport) {
			return;
		}

		constexpr float YFactor = 0.80f;
		const ImVec2 WindowSize = ImVec2(
			(std::clamp(Viewport->Size.x * 0.33f, 630.0f, 680.0f)),
			(Viewport->Size.y * YFactor));
		const ImVec2 WindowPos = ImVec2(
			(Viewport->Size.x * 0.50f) - (WindowSize.x * 0.50f),
			((Viewport->Size.y * (1.0f - YFactor)) * 0.50f));

		ImGui::SetNextWindowPos(WindowPos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(WindowSize, ImGuiCond_Always);
		constexpr int WindowFlags = ImGuiWindowFlags_NoResize
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

		const ImVec2 StartCursorPos = ImGui::GetCursorPos();
		const ImVec2 MenuSize = ImGui::GetWindowSize();
		const ImVec2 ButtonSize = {MenuSize.x, 62.0f};

		switch (UI::PauseMenu.View) {
			case UI::EPauseMenuView::Default:
				UI_PauseMenu_Default();
				break;
			case UI::EPauseMenuView::Settings:
				UI_PauseMenu_Settings();
				break;
		}

		if (UI::PauseMenu.View != UI::PauseMenu.LastView) {
			LK_TRACE_TAG("UILayer", "View changed");
		}

		UI::PauseMenu.LastView = UI::PauseMenu.View;

		UI::End();
	}

	void UI_PauseMenu_Settings()
	{
		UI::FPauseMenu::FSettings& Settings = UI::PauseMenu.Settings;
		ImGuiStyle& Style = ImGui::GetStyle();
		CWindow* Window = CWindow::Get();

		const ImVec2 StartCursorPos = ImGui::GetCursorPos();
		const ImVec2 MenuSize = ImGui::GetContentRegionAvail();
		const ImVec2 ButtonSize = {MenuSize.x, 62.0f};

		UI::ShiftCursorY(12.0f);
		UI::BannerTextCentralized("Settings", EFont::Roboto, EFontModifier::Bold);
		ImGui::Dummy(ImVec2(0, 10));

		UI::Font::Push(EFont::Roboto, EFontSize::Larger, EFontModifier::Bold);

		constexpr float PaddingX = 12.0f;
		constexpr float PaddingY = 12.0f;
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
			UI::Checkbox("Debug", UI::PauseMenu.Settings.bDebug, IndentX);

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

		ImGui::Dummy(ImVec2(0.0f, 12.0f + PaddingY * 0.50f));
		if (ImGui::Button("Style Editor", ButtonSize)) {
			Settings.bStyleEditor = !Settings.bStyleEditor;
		}
		if (ImGui::Button("ID Tool", ButtonSize)) {
			Settings.bIDStackTool = !Settings.bIDStackTool;
		}

		ImGui::PopStyleVar(2);
		UI::Font::Pop();

		/* Button: Backward */
		{
			UI::ShiftCursorY(ImGui::GetContentRegionAvail().y - ButtonSize.y);
			UI::FScopedColor ButtonHovered(ImGuiCol_ButtonHovered, RGBA32::Compliment);
			if (ImGui::Button(LK_ICON_BACKWARD, ButtonSize)) {
				UI::PauseMenu.View = UI::EPauseMenuView::Default;
			}
		}
	}

	static void UI_PauseMenu_Title(const ImVec2& MenuSize)
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

	static void UI_RenderWindows()
	{
		ImGuiStyle& Style = ImGui::GetStyle();
		UI::FPauseMenu::FSettings& Settings = UI::PauseMenu.Settings;
		if (Settings.bStyleEditor) {
			UI::FScopedFont Font(UI::Font::Get(EFont::SourceSansPro, EFontSize::Regular, EFontModifier::Normal));
			if (ImGui::Begin("Style Editor", &Settings.bStyleEditor)) {
				ImGui::ShowStyleEditor(&Style);
				ImGui::End();
			}
		}
		if (Settings.bIDStackTool) {
			ImGui::ShowIDStackToolWindow(&Settings.bIDStackTool);
		}
	}

	void UI_PauseMenu_Default()
	{
		const ImVec2 StartCursorPos = ImGui::GetCursorPos();
		const ImVec2 MenuSize = ImGui::GetContentRegionAvail();
		ImGuiStyle& Style = ImGui::GetStyle();

		/* Menu title. */
		UI_PauseMenu_Title(MenuSize);

		/* @todo: Use a table for all the menu options */

		static constexpr float OptionPercentage = 0.80f;
		const ImVec2 ButtonSize = {MenuSize.x * OptionPercentage, 62.0f};

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 14.0f);
		UI::Font::Push(EFont::Roboto, EFontSize::Header, EFontModifier::Bold);

		ImGui::SetCursorPosX(((1.0f - OptionPercentage) * 0.50f) * MenuSize.x);
		if (ImGui::Button(LK_ICON_BOOK " Main Menu", ButtonSize)) {
			Core::Global.RemoveAllLayers();
			NextMenu = CUILayer::EMenu::MainMenu;
		}

		ImGui::SetCursorPosX(((1.0f - OptionPercentage) * 0.50f) * MenuSize.x);
		if (ImGui::Button(LK_ICON_COG " Settings", ButtonSize)) {
			UI::PauseMenu.View = UI::EPauseMenuView::Settings;
		}

		ImGui::PopStyleVar(1); /* FrameRounding */

		/* Place Quit and Play buttons at the bottom. */
		ImGui::SetCursorPosY(MenuSize.y - ButtonSize.y - 2 * (Style.ItemSpacing.y + Style.FramePadding.y));

		const ImVec2 HalfButtonSize = {(ButtonSize.x * 0.50f), ButtonSize.y};

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
			UI::ClosePauseMenu();
		}
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar(1);

		UI::Font::Pop();
	}

	void CUILayer::OnKeyPressed(const FKeyData& KeyData)
	{
		LK_UNUSED(KeyData);
	}

	void CUILayer::SetActiveMenu(const EMenu InMenu)
	{
		LK_DEBUG_TAG("UILayer", "Active menu: {}", Enum::ToString(InMenu));
		NextMenu = InMenu;
	}

	void CUILayer::UI_MainMenu()
	{
		ImGuiViewport* Viewport = ImGui::GetMainViewport();
		if (!Viewport) {
			return;
		}

		constexpr float XFactor = 0.65f;
		constexpr float YFactor = 0.80f;
		constexpr float LabelColumnWidth = 180.0f;

		const ImVec2 ViewportSize = ImVec2(
			(std::clamp(Viewport->Size.x * XFactor, 620.0f, 940.0f)),
			(Viewport->Size.y * YFactor));
		const ImVec2 WindowPos = ImVec2(
			(Viewport->Size.x * 0.50f) - (ViewportSize.x * 0.50f),
			((Viewport->Size.y * (1.0f - YFactor)) * 0.50f));

		ImGui::SetNextWindowPos(WindowPos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(ViewportSize, ImGuiCond_Always);
		constexpr int WindowFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking;
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
		const ImVec2 ButtonSize = {WindowSize.x * 0.40f, 72.0f};
		UI_PauseMenu_Title(WindowSize);

		auto NextButtonEntry = []() -> void
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
		};

		/* Table: Menu Buttons */
		UI::Font::Push(EFont::Roboto, EFontSize::Header, EFontModifier::Bold);
		if (ImGui::BeginTable("##Menu", 1, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip)) {
			ImGui::TableSetupColumn("L", 0, ImGui::GetContentRegionAvail().x);

			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);

			/* Button: Levels */
			NextButtonEntry();
			{
				UI::FScopedColorStack ColorStack(
					ImGuiCol_Button, RGBA32::SmoothGreen);

				UI::ShiftCursorX((ImGui::GetContentRegionAvail().x * 0.50f) - (ButtonSize.x * 0.50f));
				if (ImGui::Button("Levels", ButtonSize)) {
					SetActiveMenu(EMenu::LevelLauncher);
				}

				ImGui::Dummy(ImVec2(0, 16));
			}

			/* Button: Editor */
			NextButtonEntry();
			{
				UI::FScopedColorStack ColorStack(
					ImGuiCol_Button, RGBA32::DarkCyan);

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
				(Avail.y - ButtonSize.y - 40.0f));
			if (ImGui::Button("Quit", ButtonSize)) {
				Core::Global.bShouldShutdown = true;
			}
			ImGui::PopStyleColor(2);
		}

		ImGui::PopStyleVar(1); /* FrameRounding */

		UI::Font::Pop();

		UI::End();
	}

	void UI::LevelLauncher()
	{
		CGameInstance* GameInstance = CGameInstance::Get();
		const bool GameInstanceValid = (GameInstance != nullptr);
		if (!GameInstanceValid) {
			UI::BeginViewport();
		}

		ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(12, 18));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 16);
		UI::Font::Push(EFont::Roboto, EFontSize::Header, EFontModifier::Bold);

		static constexpr ImVec2 ButtonSize(392.0f, 84.0f);
		const ImVec2 Avail = ImGui::GetContentRegionAvail();
		UI::ShiftCursorY(Avail.y * 0.06f);
		UI::BannerTextCentralized("Levels", EFont::SourceSansPro, EFontModifier::Bold);
		UI::ShiftCursorY(Avail.y * 0.15f);

		auto NextButtonEntry = []() -> void
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
		};

		auto EntrySpacing = []() -> void
		{
			ImGui::Dummy(ImVec2(0, 46));
		};

		if (ImGui::BeginTable("##LevelLauncher", 1, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip)) {
			ImGui::TableSetupColumn("L", 0, ImGui::GetContentRegionAvail().x);

			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);

			/* Button: Lukkelele's World */
			NextButtonEntry();
			{
				UI::FScopedColorStack ColorStack(
					ImGuiCol_Button, RGBA32::SmoothGreen);

				UI::ShiftCursorX((ImGui::GetContentRegionAvail().x * 0.50f) - (ButtonSize.x * 0.50f));
				if (ImGui::Button(LK_ICON_CODEPEN "  Lukkelele's World", ButtonSize)) {
					static const std::filesystem::path ScenePath(SCENES_DIR "/LukkelelesWorld.lscene");
					if (GameInstance) {
						GameInstance->OpenScene(ScenePath);
					} else {
						Core::Global.AddLayer(Core::ELayer::Runtime);
						NextLevel.bSelected = true;
						NextLevel.Path = ScenePath;
					}
				}

				EntrySpacing();
			}

			/* Button: TestLevel */
			NextButtonEntry();
			{
				UI::FScopedColorStack ColorStack(
					ImGuiCol_Button, RGBA32::DarkGray);

				UI::ShiftCursorX((ImGui::GetContentRegionAvail().x * 0.50f) - (ButtonSize.x * 0.50f));
				if (ImGui::Button(LK_ICON_PENCIL "  Test Level", ButtonSize)) {
					static const std::filesystem::path ScenePath(SCENES_DIR "/TestLevel.lscene");
					if (GameInstance) {
						GameInstance->OpenScene(ScenePath);
					} else {
						Core::Global.AddLayer(Core::ELayer::Editor);
						NextLevel.bSelected = true;
						NextLevel.Path = ScenePath;
					}
				}

				EntrySpacing();
			}

			ImGui::PopStyleVar(1); /* FrameRounding */

			ImGui::EndTable();
		}

		/* Button: Main Menu */
		{
			if (UI::MainMenuButton(ButtonSize)) {
				LK_TRACE_TAG("UI", "Enter main menu");
				NextMenu = CUILayer::EMenu::MainMenu;
			}
		}

		UI::Font::Pop();
		ImGui::PopStyleVar(2);

		if (!GameInstanceValid) {
			UI::EndViewport();
		}
	}

	bool UI::MainMenuButton(const ImVec2& Size)
	{
		bool Ret = false;
		UI::FScopedColorStack ColorStack(
			ImGuiCol_Button, RGBA32::DarkGray);

		const ImVec2 Avail = ImGui::GetContentRegionAvail();
		UI::ShiftCursor(
			(Avail.x * 0.50f) - (Size.x * 0.50f),
			(Avail.y - Size.y - 40.0f));
		if (ImGui::Button(LK_ICON_BOOK "  Main Menu", Size)) {
			Ret = true;
		}

		return Ret;
	}

	void UI::OpenPauseMenu(const EPauseMenuView View)
	{
		PauseMenu.View = View;
		PauseMenu.bOpen = true;
		OnPauseMenuOpened.Broadcast(PauseMenu.bOpen);
	}

	void UI::ClosePauseMenu(const EPauseMenuView View)
	{
		PauseMenu.View = View;
		PauseMenu.bOpen = false;
		OnPauseMenuOpened.Broadcast(PauseMenu.bOpen);
	}

	void UI::TogglePauseMenu()
	{
		PauseMenu.bOpen = !PauseMenu.bOpen;
		LK_TRACE_TAG("UI", "Toggle Pause Menu: {}", PauseMenu.bOpen ? "Open" : "Closed");
		OnPauseMenuOpened.Broadcast(PauseMenu.bOpen);
	}

	bool UI::IsPauseMenuOpen()
	{
		return PauseMenu.bOpen;
	}

}
