#include "uilayer.h"

#include "core/profiler.h"
#include "core/window.h"
#include "core/settings.h"
#include "game/instance.h"
#include "renderer/ui/levellauncher.h"
#include "renderer/ui/pausemenu.h"
#include "renderer/ui/ui.h"
#include "renderer/ui/widgets.h"

namespace platformer2d {

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

	void CUILayer::UI_MainMenu()
	{
		ImGuiViewport* Viewport = ImGui::GetMainViewport();
		if (!Viewport) {
			return;
		}

		constexpr float XFactor = 0.65f;
		constexpr float YFactor = 0.80f;

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

		const ImVec2 WindowSize = ImGui::GetWindowSize();
		const ImVec2 ButtonSize = {WindowSize.x * 0.40f, 72.0f};
		UI::DrawMenuTitle(WindowSize);

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

}
