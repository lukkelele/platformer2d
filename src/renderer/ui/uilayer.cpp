#include "uilayer.h"

#include <array>

#include "core/profiler.h"
#include "core/window.h"
#include "core/settings.h"
#include "game/instance.h"
#include "renderer/color.h"
#include "renderer/ui/levellauncher.h"
#include "renderer/ui/mainmenustyle.h"
#include "renderer/ui/pausemenu.h"
#include "renderer/ui/settingsmenu.h"
#include "renderer/ui/ui.h"
#include "renderer/ui/widgets.h"

namespace platformer2d {

	static UI::FSettingsMenuState MainMenuSettingsState{};

	CUILayer::CUILayer(std::string_view InName)
		: CLayer(InName)
	{
		NextMenu = ActiveMenu;
	}

	void CUILayer::Initialize()
	{
		ImGuiLayer = std::make_unique<CImGuiLayer>(CWindow::Get().GetGlfwWindow());

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
		DelegateHandles.OnKey = CKeyboard::OnKeyEvent.Add(this, &CUILayer::OnKey);
	}

	void CUILayer::OnDetach()
	{
		LK_TRACE_TAG("UILayer", "OnDetach");
		CKeyboard::OnKeyEvent.Remove(DelegateHandles.OnKey);
		if (ImGuiLayer) {
			LK_DEBUG_TAG("UILayer", "Destroy ImGui layer");
			ImGuiLayer->Destroy();
			ImGuiLayer.reset();
		}
	}

	void CUILayer::Tick(const float DeltaTime)
	{
		LK_PROFILE_FUNC();
		UI::TickLevelLauncher();
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

		if (CGameInstance::IsValid()) {
			auto& GameInstance = CGameInstance::Get();
			if (GameInstance.HasScene()) {
				UI::DrawPauseMenu();
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
				case EMenu::Settings:
					UI_Settings();
					break;
				case EMenu::Credits:
					UI_Credits();
					break;
			}
		}

		ActiveMenu = NextMenu;
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

	void CUILayer::OnKey(const FKeyData& KeyData)
	{
		LK_UNUSED(KeyData);
	}

	void CUILayer::SetActiveMenu(const EMenu InMenu)
	{
		LK_DEBUG_TAG("UILayer", "Active menu: {}", Enum::ToString(InMenu));
		NextMenu = InMenu;
	}

	void CUILayer::RequestMenu(const EMenu InMenu)
	{
		NextMenu = InMenu;
	}

	CUILayer::EMenu CUILayer::GetActiveMenu()
	{
		return ActiveMenu;
	}

	namespace {
		struct FMainMenuEntry
		{
			CUILayer::EMenu TargetMenu = CUILayer::EMenu::None;
			const char* Icon = nullptr;
			const char* Label = nullptr;
			const char* Shortcut = nullptr;
		};
	}

	static bool DrawMainMenuButton(const FMainMenuEntry& Entry, const bool Selected, const ImVec2& Size)
	{
		static const std::uint32_t ButtonBgHoverColor = FColor::Convert<std::uint32_t>(glm::vec4(1.0f, 1.0f, 1.0f, 0.100f));
		static const std::uint32_t ButtonBgActiveColor = FColor::Convert<std::uint32_t>(glm::vec4(1.0f, 1.0f, 1.0f, 0.200f));
		static const std::uint32_t ButtonBgColor = FColor::Convert<std::uint32_t>(glm::vec4(0.30f, 0.30f, 0.30f, 0.90f));
		UI::FScopedColor ButtonBg(ImGuiCol_Button, ButtonBgColor);
		UI::FScopedColor ButtonHover(ImGuiCol_ButtonHovered, ButtonBgHoverColor);
		UI::FScopedColor ButtonActive(ImGuiCol_ButtonActive, ButtonBgActiveColor);
		UI::FScopedColor TextColor(ImGuiCol_Text, RGBA32::White);
		UI::FScopedStyle ButtonPadding(ImGuiStyleVar_FramePadding, ImVec2(28, 12));
		UI::FScopedStyle ButtonRounding(ImGuiStyleVar_FrameRounding, 4.0f);

		ImGui::PushID(Entry.Label);
		const ImVec2 CursorBefore = ImGui::GetCursorScreenPos();
		const bool Clicked = ImGui::Button("##MainMenuButton", Size);
		ImGui::PopID();

		constexpr float PADDING_LEFT = 28.0f;
		constexpr float ARROW_TO_LABEL_GAP = 12.0f;
		constexpr float ARROW_Y_OFFSET = -3.0f;

		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		const std::uint32_t LabelColor = ImGui::GetColorU32(ImGuiCol_Text);

		const bool Hovered = UI::IsItemHovered(0.0f);
		const char* Arrow = (Selected && Hovered) ? LK_ICON_ARROW_RIGHT : " ";
		const ImVec2 ArrowSize = ImGui::CalcTextSize(Arrow);
		const ImVec2 ArrowPos(CursorBefore.x + PADDING_LEFT, CursorBefore.y + (Size.y - ArrowSize.y) * 0.50f + ARROW_Y_OFFSET);
		DrawList->AddText(ArrowPos, LabelColor, Arrow);

		const ImVec2 LabelSize = ImGui::CalcTextSize(Entry.Label);
		const ImVec2 LabelPos(CursorBefore.x + PADDING_LEFT + ArrowSize.x + ARROW_TO_LABEL_GAP,
			CursorBefore.y + (Size.y - LabelSize.y) * 0.50f);
		DrawList->AddText(LabelPos, LabelColor, Entry.Label);

		if (Entry.Shortcut) {
			const ImVec2 ShortcutSize = ImGui::CalcTextSize(Entry.Shortcut);
			const ImVec2 ShortcutPos(CursorBefore.x + Size.x - ShortcutSize.x - 24.0f,
				CursorBefore.y + (Size.y - ShortcutSize.y) * 0.50f);
			DrawList->AddText(ShortcutPos, RGBA32::Text::Darker, Entry.Shortcut);
		}

		return Clicked;
	}

	void CUILayer::UI_MainMenu()
	{
		LK_PROFILE_FUNC();
		ImGuiViewport* Viewport = ImGui::GetMainViewport();
		if (!Viewport) {
			return;
		}

		UI::DrawMainMenuBackground(Viewport);

		ImGui::SetNextWindowPos(Viewport->Pos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(Viewport->Size, ImGuiCond_Always);
		constexpr ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoTitleBar
			| ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoCollapse
			| ImGuiWindowFlags_NoDocking
			| ImGuiWindowFlags_NoBringToFrontOnFocus
			| ImGuiWindowFlags_NoScrollbar
			| ImGuiWindowFlags_NoScrollWithMouse;

		static const std::uint32_t ButtonBgTransparent = FColor::Convert<std::uint32_t>(FColor::Transparent);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ButtonBgTransparent);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		const bool WindowOpened = UI::Begin("##MainMenuBase", nullptr, WindowFlags);
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(1);
		if (!WindowOpened) {
			return;
		}

		const ImVec2 Avail = Viewport->Size;
		const float ContentX = std::clamp(Avail.x * 0.08f, 48.0f, 120.0f);
		const float ContentY = std::clamp(Avail.y * 0.18f, 60.0f, 140.0f);

		ImGui::SetCursorPos(ImVec2(ContentX, ContentY));
		UI::Font::Push(EFont::Roboto, EFontSize::Banner, EFontModifier::BoldItalic);
		UI::Text::Shimmer("platformer2d", FColor::Convert<ImVec4>(FColor::LightGray), FColor::Convert<ImVec4>(FColor::White), 140.0f);
		UI::Font::Pop();

		ImGui::SetCursorPosX(ContentX);
		UI::Font::Push(EFont::Roboto, EFontSize::Regular, EFontModifier::BoldItalic);
		ImGui::TextColored(ImColor(RGBA32::Text::Darker), "made by lukkelele");
		UI::Font::Pop();

		ImGui::Dummy(ImVec2(0, 36));

		static const std::array<FMainMenuEntry, 6> Entries = {
			{
             {EMenu::LevelLauncher, LK_ICON_PLAY, "Play", nullptr},
             {EMenu::LevelLauncher, LK_ICON_LIST, "Levels", nullptr},
             {EMenu::None, LK_ICON_PENCIL, "Editor", nullptr},
             {EMenu::Settings, LK_ICON_COG, "Settings", nullptr},
             {EMenu::Credits, LK_ICON_BOOK, "Credits", nullptr},
             {EMenu::None, LK_ICON_POWER_OFF, "Quit", nullptr},
			 }
        };

		const ImVec2 ButtonSize(std::min(420.0f, Avail.x * 0.30f), 62.0f);
		UI::Font::Push(EFont::Roboto, EFontSize::Header, EFontModifier::Bold);

		static std::size_t HoveredIdx = std::numeric_limits<std::size_t>::max();
		for (std::size_t Idx = 0; Idx < Entries.size(); Idx++) {
			ImGui::SetCursorPosX(ContentX);
			const bool Hovered = (HoveredIdx == Idx);
			if (DrawMainMenuButton(Entries[Idx], Hovered, ButtonSize)) {
				switch (Idx) {
					case 0: /* Play */
						[[fallthrough]];
					case 1: /* Levels */
						SetActiveMenu(EMenu::LevelLauncher);
						break;
					case 2: /* Editor */
						Core::Global.AddLayer(Core::ELayer::Editor);
						break;
					case 3: /* Settings */
						SetActiveMenu(EMenu::Settings);
						break;
					case 4: /* Credits */
						SetActiveMenu(EMenu::Credits);
						break;
					case 5: /* Quit */
						Core::Global.bShouldShutdown = true;
						break;
				}
			}
			if (ImGui::IsItemHovered()) {
				HoveredIdx = Idx;
			}
		}

		UI::Font::Pop();

		/* Version footer. */
		{
			UI::Font::Push(EFont::Roboto, EFontSize::Regular, EFontModifier::Normal);
			const char* VersionText = "v0.1.0   build: dev"; /* @todo: Proper version */
			const char* CreditText = "(c) 2026 Lukas Gunnarsson";
			const ImVec2 VersionSize = ImGui::CalcTextSize(VersionText);
			const ImVec2 CreditSize = ImGui::CalcTextSize(CreditText);
			const float FooterY = Avail.y - VersionSize.y - 24.0f;

			ImGui::SetCursorPos(ImVec2(ContentX, FooterY));
			ImGui::TextColored(ImColor(RGBA32::Text::Darker), "%s", VersionText);

			ImGui::SetCursorPos(ImVec2(Avail.x - CreditSize.x - ContentX, FooterY));
			ImGui::TextColored(ImColor(RGBA32::Text::Disabled), "%s", CreditText);
			UI::Font::Pop();
		}

		UI::End();
	}

	void CUILayer::UI_Settings()
	{
		LK_PROFILE_FUNC();
		ImGuiViewport* Viewport = ImGui::GetMainViewport();
		if (!Viewport) {
			return;
		}

		UI::DrawMainMenuBackground(Viewport);

		const ImVec2 Avail = Viewport->Size;
		const float WindowWidth = std::clamp(Avail.x * 0.70f, 820.0f, 1180.0f);
		const float WindowHeight = std::clamp(Avail.y * 0.80f, 540.0f, 820.0f);
		const ImVec2 WindowPos((Avail.x - WindowWidth) * 0.50f, (Avail.y - WindowHeight) * 0.50f);

		ImGui::SetNextWindowPos(WindowPos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(WindowWidth, WindowHeight), ImGuiCond_Always);

		constexpr ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoDecoration
			| ImGuiWindowFlags_NoDocking
			| ImGuiWindowFlags_NoCollapse
			| ImGuiWindowFlags_NoMove;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 16.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, RGBA32::BackgroundDark);
		ImGui::PushStyleColor(ImGuiCol_Border, RGBA32::Muted);

		const bool WindowOpened = UI::Begin("##MainMenuSettings", nullptr, WindowFlags);
		ImGui::PopStyleVar(3);
		ImGui::PopStyleColor(2);
		if (!WindowOpened) {
			return;
		}

		const ImVec2 BodyAvail = ImGui::GetContentRegionAvail();
		constexpr float BackButtonHeight = 48.0f;
		constexpr float SaveButtonWidth = 200.0f;
		constexpr float ButtonSpacing = 8.0f;
		const float ContentHeight = BodyAvail.y - BackButtonHeight - ButtonSpacing;

		ImGui::BeginChild("##MainMenuSettingsBody", ImVec2(BodyAvail.x, ContentHeight),
			ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		UI::DrawSettingsPanel(MainMenuSettingsState);
		ImGui::EndChild();

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
		const float BackWidth = std::max(120.0f, BodyAvail.x - SaveButtonWidth - ButtonSpacing);
		{
			UI::FScopedColor ButtonHovered(ImGuiCol_ButtonHovered, RGBA32::Compliment);
			if (ImGui::Button(LK_ICON_BACKWARD "  Back", ImVec2(BackWidth, BackButtonHeight))) {
				SetActiveMenu(EMenu::MainMenu);
			}
		}
		ImGui::SameLine(0.0f, ButtonSpacing);
		UI::DrawSaveButton(MainMenuSettingsState, ImVec2(SaveButtonWidth, BackButtonHeight));
		ImGui::PopStyleVar(1);

		UI::End();

		UI::DrawSettingsDebugTools(MainMenuSettingsState);
	}

	void CUILayer::UI_Credits()
	{
		LK_PROFILE_FUNC();
		ImGuiViewport* Viewport = ImGui::GetMainViewport();
		if (!Viewport) {
			return;
		}

		UI::DrawMainMenuBackground(Viewport);

		const ImVec2 Avail = Viewport->Size;
		const float WindowWidth = std::clamp(Avail.x * 0.45f, 520.0f, 720.0f);
		const float WindowHeight = std::clamp(Avail.y * 0.65f, 420.0f, 660.0f);
		const ImVec2 WindowPos((Avail.x - WindowWidth) * 0.50f, (Avail.y - WindowHeight) * 0.50f);

		ImGui::SetNextWindowPos(WindowPos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(WindowWidth, WindowHeight), ImGuiCond_Always);

		constexpr ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoDecoration
			| ImGuiWindowFlags_NoDocking
			| ImGuiWindowFlags_NoCollapse
			| ImGuiWindowFlags_NoMove;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(28.0f, 28.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, RGBA32::BackgroundDark);
		ImGui::PushStyleColor(ImGuiCol_Border, RGBA32::Muted);
		const bool WindowOpened = UI::Begin("##Credits", nullptr, WindowFlags);
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar(3);
		if (!WindowOpened) {
			return;
		}

		UI::BannerTextCentralized("Credits", EFont::Roboto, EFontModifier::Bold);
		ImGui::Dummy(ImVec2(0, 16));

		auto Section = [](const char* Header, const char* Value)
		{
			UI::Font::Push(EFont::Roboto, EFontSize::Regular, EFontModifier::Bold);
			ImGui::TextColored(ImColor(RGBA32::Compliment), "%s", Header);
			UI::Font::Pop();
			UI::Font::Push(EFont::Roboto, EFontSize::Large, EFontModifier::Normal);
			ImGui::TextColored(ImColor(RGBA32::Text::Brighter), "  %s", Value);
			UI::Font::Pop();
			ImGui::Dummy(ImVec2(0, 10));
		};

		Section("Created by", "Lukas Gunnarsson");
		Section("Engine", "platformer2d (C++23)");
		Section("Rendering", "OpenGL 4.6 + ImGui");
		Section("Physics", "Box2D");

		ImGui::Dummy(ImVec2(0, 16));

		const ImVec2 BodyAvail = ImGui::GetContentRegionAvail();
		const float ButtonHeight = 44.0f;
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + std::max(0.0f, BodyAvail.y - ButtonHeight));

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
		UI::FScopedColor ButtonHovered(ImGuiCol_ButtonHovered, RGBA32::Compliment);
		if (ImGui::Button(LK_ICON_BACKWARD "  Back", ImVec2(-1.0f, ButtonHeight))) {
			SetActiveMenu(EMenu::MainMenu);
		}
		ImGui::PopStyleVar(1);

		UI::End();
	}

}
