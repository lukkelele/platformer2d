#include "settingsmenu.h"

#include <array>

#include "core/core.h"
#include "core/enum.h"
#include "core/settings.h"
#include "core/window.h"
#include "game/instance.h"
#include "renderer/camera.h"
#include "renderer/color.h"
#include "renderer/font.h"
#include "renderer/renderer.h"
#include "renderer/ui/combo.h"
#include "renderer/ui/scoped.h"
#include "renderer/ui/ui.h"
#include "renderer/ui/ui_core.h"
#include "renderer/ui/widgets.h"

namespace platformer2d::UI {

	static constexpr float OPTION_WIDTH = 220.0f;
	static constexpr float CATEGORY_PANE_WIDTH = 184.0f;
	static constexpr float SETTINGS_LABEL_INDENT = 20.0f;
	static constexpr float SETTINGS_VALUE_INDENT = 24.0f;

	namespace {
		struct FCategoryEntry
		{
			ESettingsCategory Category;
			const char* Icon;
			const char* Label;
		};

		struct FResolutionPreset
		{
			std::uint16_t Width;
			std::uint16_t Height;
			const char* Label;
		};
	}

	static const std::array<FCategoryEntry, std::to_underlying(ESettingsCategory::COUNT)> CategoryEntries = {
		{
         {ESettingsCategory::Window, LK_ICON_DESKTOP, "Window"},
         {ESettingsCategory::Graphics, LK_ICON_PICTURE_O, "Graphics"},
         {ESettingsCategory::Input, LK_ICON_KEYBOARD_O, "Input"},
         {ESettingsCategory::Gameplay, LK_ICON_GAMEPAD, "Gameplay"},
         {ESettingsCategory::Renderer, LK_ICON_PAINT_BRUSH, "Renderer"},
         {ESettingsCategory::Camera, LK_ICON_VIDEO_CAMERA, "Camera"},
         {ESettingsCategory::Debug, LK_ICON_BUG, "Debug"},
		 }
    };

	static constexpr std::array<FResolutionPreset, 7> ResolutionPresets = {
		{
         {1280, 720, "1280 x 720"},
         {1366, 768, "1366 x 768"},
         {1600, 900, "1600 x 900"},
         {1920, 1080, "1920 x 1080"},
         {2560, 1440, "2560 x 1440"},
         {3440, 1440, "3440 x 1440"},
         {3840, 2160, "3840 x 2160"},
		 }
    };

	static void BeginSection(const char* Label, const float ContentIndent = SETTINGS_VALUE_INDENT)
	{
		UI::ShiftCursorX(SETTINGS_LABEL_INDENT);
		ImGui::PushStyleColor(ImGuiCol_Text, RGBA32::Text::Darker);
		UI::HeaderText(Label, EFont::Roboto, EFontModifier::Bold);
		ImGui::PopStyleColor(1);
		UI::ShiftCursor(ContentIndent, 8.0f);
	}

	static bool BeginSettingsTable(const char* TableID)
	{
		const ImVec2 Avail = ImGui::GetContentRegionAvail();
		if (!ImGui::BeginTable(TableID, 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip)) {
			return false;
		}

		ImGui::TableSetupColumn("Label", 0, Avail.x * 0.45f);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip,
			ImGui::GetContentRegionAvail().x * 0.55f);
		return true;
	}

	static void DrawCategorySidebar(FSettingsMenuState& State)
	{
		const ImVec2 PaneSize(CATEGORY_PANE_WIDTH, ImGui::GetContentRegionAvail().y);
		UI::FScopedColor ChildBg(ImGuiCol_ChildBg, RGBA32::BackgroundDarker);
		UI::FScopedStyle ChildRounding(ImGuiStyleVar_ChildRounding, 8.0f);
		UI::FScopedStyle WindowPadding(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 12.0f));
		ImGui::BeginChild("##SettingsCategories", PaneSize, ImGuiChildFlags_None,
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		UI::Font::Push(EFont::Roboto, EFontSize::Large, EFontModifier::Bold);

		constexpr float OUTER_PAD_X = 8.0f;
		constexpr float INNER_PAD_X = 14.0f;
		constexpr float ICON_TO_LABEL_GAP = 12.0f;
		constexpr float ITEM_SPACING_Y = 4.0f;
		constexpr float ROUNDING = 6.0f;
		constexpr float ICON_OFFSET_Y = 2.0f;

		const float ButtonHeight = ImGui::GetFontSize() + 20.0f;
		const ImVec2 ButtonSize(PaneSize.x - OUTER_PAD_X * 2.0f, ButtonHeight);
		const std::uint32_t TransparentBg = FColor::Convert<std::uint32_t>(FColor::Transparent);
		const std::uint32_t TextColor = ImGui::GetColorU32(ImGuiCol_Text);

		for (std::size_t Idx = 0; Idx < CategoryEntries.size(); Idx++) {
			const FCategoryEntry& Entry = CategoryEntries[Idx];
			const bool bSelected = (State.Category == Entry.Category);

			UI::ShiftCursorX(OUTER_PAD_X);
			ImGui::PushID(Entry.Label);
			const ImVec2 ButtonPos = ImGui::GetCursorScreenPos();
			const bool Clicked = ImGui::InvisibleButton("##cat", ButtonSize);
			const bool Hovered = ImGui::IsItemHovered();
			const bool Active = ImGui::IsItemActive();
			ImGui::PopID();

			ImDrawList* DrawList = ImGui::GetWindowDrawList();
			const ImVec2 ButtonMax(ButtonPos.x + ButtonSize.x, ButtonPos.y + ButtonSize.y);
			const std::uint32_t BgColor = (Active || bSelected) ? RGBA32::Compliment
																: (Hovered ? RGBA32::Muted : TransparentBg);
			if (BgColor != TransparentBg) {
				DrawList->AddRectFilled(ButtonPos, ButtonMax, BgColor, ROUNDING);
			}

			const ImVec2 IconSize = ImGui::CalcTextSize(Entry.Icon);
			const ImVec2 LabelSize = ImGui::CalcTextSize(Entry.Label);
			const float IconX = ButtonPos.x + INNER_PAD_X;
			const float IconY = ButtonPos.y + (ButtonSize.y - IconSize.y) * 0.50f + ICON_OFFSET_Y;
			const float LabelX = IconX + IconSize.x + ICON_TO_LABEL_GAP;
			const float LabelY = ButtonPos.y + (ButtonSize.y - LabelSize.y) * 0.50f;
			DrawList->AddText(ImVec2(IconX, IconY), TextColor, Entry.Icon);
			DrawList->AddText(ImVec2(LabelX, LabelY), TextColor, Entry.Label);

			if (Clicked) {
				State.Category = Entry.Category;
			}

			if ((Idx + 1) < CategoryEntries.size()) {
				ImGui::Dummy(ImVec2(0, ITEM_SPACING_Y));
			}
		}
		UI::Font::Pop();
		ImGui::EndChild();
	}

	static void DrawCategory_Window(FSettings& AppSettings)
	{
		BeginSection("Window");
		if (BeginSettingsTable("##WindowTable")) {
			const ImVec2 Avail = ImGui::GetContentRegionAvail();
			constexpr float IndentX = 0.0f;

			UI::Table::NextRow();
			if (UI::Checkbox("VSync", AppSettings.Window.bVSync, IndentX)) {
				CWindow::Get().SetVSync(AppSettings.Window.bVSync);
			}

			UI::Table::NextRow();
			if (UI::Checkbox("Start Maximized", AppSettings.Window.bStartMaximized, IndentX)) {
				if (AppSettings.Window.bStartMaximized) {
					CWindow::Get().Maximize();
				} else {
					CWindow::Get().Restore();
				}
			}

			UI::Table::NextRow();
			UI::Table::Label("Resolution");
			UI::Table::NextColumn();

			std::array<char, 32> Current = {0};
			const CWindow& Window = CWindow::Get();
			std::snprintf(Current.data(), Current.size(), "%u x %u", Window.GetWidth(), Window.GetHeight());

			const std::uint16_t WindowW = Window.GetWidth();
			const std::uint16_t WindowH = Window.GetHeight();
			ImGui::SetNextItemWidth(OPTION_WIDTH);
			if (ImGui::BeginCombo("##Resolution", Current.data())) {
				for (const FResolutionPreset& Preset : ResolutionPresets) {
					const bool IsSelected = ((Preset.Width == WindowW) && (Preset.Height == WindowH));
					if (ImGui::Selectable(Preset.Label, IsSelected)) {
						CWindow::Get().SetSize(Preset.Width, Preset.Height);
					}
					if (IsSelected) {
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			UI::Table::NextRow();
			UI::Table::Label("Refresh Rate");
			UI::Table::NextColumn();
			ImGui::Text("%u Hz", Window.GetRefreshRate());

			ImGui::EndTable();
		}
	}

	static void DrawCategory_Graphics(FSettings& AppSettings)
	{
		BeginSection("Graphics");
		if (BeginSettingsTable("##GraphicsTable")) {
			const ImVec2 Avail = ImGui::GetContentRegionAvail();
			constexpr float IndentX = 0.0f;

			UI::Table::NextRow();
			UI::Checkbox("Show FPS", AppSettings.Graphics.bShowFPS, IndentX);

			UI::Table::NextRow();
			UI::Checkbox("Show Frametime", AppSettings.Graphics.bShowFrametime, IndentX);

			UI::Table::NextRow();
			UI::Checkbox("Show Debug Stats", AppSettings.Graphics.bShowDebugStats, IndentX);

			UI::Table::NextRow();
			UI::Table::Label("Brightness");
			UI::Table::NextColumn();
			ImGui::SetNextItemWidth(OPTION_WIDTH);
			ImGui::SliderFloat("##Brightness", &AppSettings.Graphics.Brightness, 0.50f, 2.0f, "%.2f");

			UI::Table::NextRow();
			UI::Table::Label("UI Scale");
			UI::Table::NextColumn();
			ImGui::SetNextItemWidth(OPTION_WIDTH);
			ImGui::SliderFloat("##UIScale", &AppSettings.Graphics.UIScale, 0.50f, 2.50f, "%.2f");

			ImGui::EndTable();
		}
	}

	static void DrawCategory_Input(FSettings& AppSettings)
	{
		BeginSection("Input");
		if (BeginSettingsTable("##InputTable")) {
			const ImVec2 Avail = ImGui::GetContentRegionAvail();
			const float IndentX = 0.0f;

			UI::Table::NextRow();
			UI::Table::Label("Mouse Sensitivity");
			UI::Table::NextColumn();
			ImGui::SetNextItemWidth(OPTION_WIDTH);
			ImGui::SliderFloat("##MouseSensitivity", &AppSettings.Input.MouseSensitivity, 0.10f, 4.0f, "%.2f");

			UI::Table::NextRow();
			UI::Table::Label("Zoom Speed");
			UI::Table::NextColumn();
			ImGui::SetNextItemWidth(OPTION_WIDTH);
			ImGui::SliderFloat("##ZoomSpeed", &AppSettings.Input.ZoomSpeed, 0.10f, 4.0f, "%.2f");

			UI::Table::NextRow();
			UI::Checkbox("Invert Camera Drag", AppSettings.Input.bInvertCameraDrag, IndentX);

			UI::Table::NextRow();
			UI::Checkbox("Edge Pan", AppSettings.Input.bEdgePan, IndentX);

			if (AppSettings.Input.bEdgePan) {
				UI::Table::NextRow();
				UI::Table::Label("Edge Pan Speed");
				UI::Table::NextColumn();
				ImGui::SetNextItemWidth(OPTION_WIDTH);
				ImGui::SliderFloat("##EdgePanSpeed", &AppSettings.Input.EdgePanSpeed, 1.0f, 32.0f, "%.1f");
			}

			ImGui::EndTable();
		}

		ImGui::Dummy(ImVec2(0, 16));
		BeginSection("Bindings");
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
		if (ImGui::BeginTable("##BindingsTable", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip | ImGuiTableFlags_RowBg)) {
			const ImVec2 Avail = ImGui::GetContentRegionAvail();
			ImGui::TableSetupColumn("Action", 0, Avail.x * 0.55f);
			ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip,
				Avail.x * 0.45f);

			static constexpr std::array<std::pair<const char*, const char*>, 7> Bindings = {
				{
                 {"Move Left", "A / Left"},
                 {"Move Right", "D / Right"},
                 {"Jump", "Space"},
                 {"Crouch", "S / Down"},
                 {"Fire / Aim", "Mouse 1"},
                 {"Pause Menu", "Escape"},
                 {"Toggle Editor", "F1"},
				 }
            };

			for (const auto& [Action, Key] : Bindings) {
				UI::Table::NextRow();
				UI::Table::Label(Action);
				UI::Table::NextColumn();
				UI::FScopedColor TextColor(ImGuiCol_Text, RGBA32::Text::Brighter);
				ImGui::Text("%s", Key);
			}
			ImGui::EndTable();
		}
		ImGui::PopStyleVar(1);
	}

	static void DrawCategory_Gameplay(FSettings& AppSettings)
	{
		BeginSection("Gameplay");
		if (!BeginSettingsTable("##GameplayTable")) {
			return;
		}
		const ImVec2 Avail = ImGui::GetContentRegionAvail();
		const float IndentX = 0.0f;

		UI::Table::NextRow();
		UI::Table::Label("Difficulty");
		UI::Table::NextColumn();
		ImGui::SetNextItemWidth(OPTION_WIDTH);
		EDifficulty Difficulty = AppSettings.Gameplay.Difficulty;
		if (UI::Combo("##Difficulty", Enum::View<EDifficulty>(), Difficulty)) {
			AppSettings.Gameplay.Difficulty = Difficulty;
		}

		UI::Table::NextRow();
		UI::Checkbox("Show Hints", AppSettings.Gameplay.bShowTutorialHints, IndentX);

		UI::Table::NextRow();
		UI::Checkbox("Screen Shake", AppSettings.Gameplay.bScreenShake, IndentX);

		UI::Table::NextRow();
		UI::Table::Label("Master Scale");
		UI::Table::NextColumn();
		ImGui::SetNextItemWidth(OPTION_WIDTH);
		ImGui::SliderFloat("##MasterScale", &AppSettings.Gameplay.MasterScale, 0.50f, 2.0f, "%.2f");

		ImGui::EndTable();
	}

	static void DrawCategory_Renderer()
	{
		BeginSection("Renderer");
		if (BeginSettingsTable("##RendererTable")) {
			const ImVec2 Avail = ImGui::GetContentRegionAvail();
			const float IndentX = 0.0f;

			UI::Table::NextRow();
			bool bDepthTest = CRenderer::GetDepthTest();
			if (UI::Checkbox("Depth Test", bDepthTest, IndentX)) {
				CRenderer::SetDepthTest(bDepthTest);
			}

			UI::Table::NextRow();
			ImGui::TableSetColumnIndex(0);
			UI::BlendFunction();

			UI::Table::NextRow();
			ImGui::TableSetColumnIndex(0);
			UI::DepthFunction();

			ImGui::EndTable();
		}
	}

	static void DrawCategory_Camera()
	{
		BeginSection("Camera");
		if (!CGameInstance::IsValid()) {
			UI::FScopedColor TextColor(ImGuiCol_Text, RGBA32::Text::Darker);
			ImGui::TextWrapped("Camera options are only available while a scene is loaded.");
			return;
		}

		CCamera* Camera = CGameInstance::Get().GetActiveCamera();
		if (Camera == nullptr) {
			UI::FScopedColor TextColor(ImGuiCol_Text, RGBA32::Text::Darker);
			ImGui::TextWrapped("No active camera");
			return;
		}

		if (BeginSettingsTable("##CameraTable")) {
			UI::Table::NextRow();
			UI::Table::Label("Zoom");
			UI::Table::NextColumn();
			ImGui::SetNextItemWidth(280.0f);
			float Zoom = Camera->GetZoom();
			if (ImGui::SliderFloat("##CameraZoom", &Zoom, CCamera::ZOOM_MIN, CCamera::ZOOM_MAX, "%.2f")) {
				if (ImGui::IsItemActive()) {
					Camera->SetZoom(Zoom);
				}
			}

			UI::Table::NextRow();
			UI::Table::Label("Position");
			UI::Table::NextColumn();
			const glm::vec2 Pos = Camera->GetPosition();
			ImGui::Text("(%.2f, %.2f)", Pos.x, Pos.y);

			UI::Table::NextRow();
			UI::Table::Label("Viewport");
			UI::Table::NextColumn();
			ImGui::Text("%.0f x %.0f", Camera->GetViewportWidth(), Camera->GetViewportHeight());

			ImGui::EndTable();
		}
	}

	static void DrawCategory_Debug(FSettingsMenuState& State)
	{
		BeginSection("Debug Tools", 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 6.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

		const ImVec2 Avail = ImGui::GetContentRegionAvail();
		const ImVec2 ButtonSize(Avail.x * 0.85f, 40.0f);

		if (ImGui::Button(LK_ICON_PAINT_BRUSH "  Style Editor", ButtonSize)) {
			State.bStyleEditor = !State.bStyleEditor;
		}
		if (ImGui::Button(LK_ICON_LIST "  ID Stack Tool", ButtonSize)) {
			State.bIDStackTool = !State.bIDStackTool;
		}
		if (ImGui::Button(LK_ICON_TH_LARGE "  ImGui Debug Borders", ButtonSize)) {
			State.bDebugBorders = !State.bDebugBorders;
			ImGui::GetStyle().Colors[ImGuiCol_Border] = State.bDebugBorders
				? ImVec4(1.0f, 0.0f, 1.0f, 1.0f)
				: ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
		}

		ImGui::PopStyleVar(2);
	}

	bool DrawSaveButton(FSettingsMenuState& State, const ImVec2& Size)
	{
		constexpr float SAVE_FLASH_DURATION = 1.50f;

		bool Clicked = false;
		{
			UI::FScopedColor ButtonBg(ImGuiCol_Button, RGBA32::NiceGreen);
			UI::FScopedColor ButtonHovered(ImGuiCol_ButtonHovered, IM_COL32(0, 205, 15, 90));
			UI::FScopedStyle ButtonRounding(ImGuiStyleVar_FrameRounding, 8.0f);
			Clicked = ImGui::Button(LK_ICON_FLOPPY_O "  Save", Size);
		}

		if (Clicked) {
			State.bLastSaveOk = FSettings::Save();
			State.SaveFlashTime = SAVE_FLASH_DURATION;
			if (State.bLastSaveOk) {
				LK_INFO_TAG("Settings", "Saved settings to: {}", FSettings::GetFilePath());
			} else {
				LK_ERROR_TAG("Settings", "Failed to save settings to: {}", FSettings::GetFilePath());
			}
		}

		if (State.SaveFlashTime > 0.0f) {
			const float Alpha = std::clamp(State.SaveFlashTime / SAVE_FLASH_DURATION, 0.0f, 1.0f);
			const ImVec4 Color = State.bLastSaveOk
				? ImVec4(0.40f, 0.95f, 0.40f, Alpha)
				: ImVec4(0.95f, 0.40f, 0.40f, Alpha);
			ImGui::SameLine(0.0f, 12.0f);
			ImGui::PushStyleColor(ImGuiCol_Text, Color);
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(State.bLastSaveOk ? LK_ICON_CHECK "  Saved" : LK_ICON_TIMES "  Save failed");
			ImGui::PopStyleColor(1);
			State.SaveFlashTime -= ImGui::GetIO().DeltaTime;
		}

		return Clicked;
	}

	void DrawSettingsPanel(FSettingsMenuState& State)
	{
		FSettings& Settings = FSettings::Get();

		UI::ShiftCursorY(8.0f);
		UI::BannerTextCentralized("Settings", EFont::Roboto, EFontModifier::Bold);
		ImGui::Dummy(ImVec2(0, 6));

		const ImVec2 BodySize = ImGui::GetContentRegionAvail();
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12.0f, 0.0f));
		if (ImGui::BeginTable("##SettingsLayout", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip)) {
			ImGui::TableSetupColumn("Categories", ImGuiTableColumnFlags_WidthFixed, CATEGORY_PANE_WIDTH);
			ImGui::TableSetupColumn("Content", ImGuiTableColumnFlags_WidthStretch, BodySize.x - CATEGORY_PANE_WIDTH);

			UI::Table::NextRow();
			ImGui::TableSetColumnIndex(0);
			DrawCategorySidebar(State);

			ImGui::TableSetColumnIndex(1);
			{
				UI::FScopedColor ChildBg(ImGuiCol_ChildBg, RGBA32::BackgroundDark);
				UI::FScopedStyle ChildRounding(ImGuiStyleVar_ChildRounding, 8.0f);
				UI::FScopedStyle WindowPadding(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 14.0f));
				ImGuiWindowFlags ContentFlags = ImGuiWindowFlags_AlwaysVerticalScrollbar;
				if (!State.bContentScrollable) {
					ContentFlags |= ImGuiWindowFlags_NoScrollWithMouse;
				}

				const bool ScrollbarHidden = !State.bContentScrollable;
				if (ScrollbarHidden) {
					const std::uint32_t Transparent = FColor::Convert<std::uint32_t>(FColor::Transparent);
					ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, Transparent);
					ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, Transparent);
					ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, Transparent);
					ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, Transparent);
				}
				ImGui::BeginChild("##SettingsContent", ImVec2(0, BodySize.y), ImGuiChildFlags_None, ContentFlags);

				UI::Font::Push(EFont::Roboto, EFontSize::Larger, EFontModifier::Bold);
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

				switch (State.Category) {
					case ESettingsCategory::Window:   DrawCategory_Window(Settings); break;
					case ESettingsCategory::Graphics: DrawCategory_Graphics(Settings); break;
					case ESettingsCategory::Input:    DrawCategory_Input(Settings); break;
					case ESettingsCategory::Gameplay: DrawCategory_Gameplay(Settings); break;
					case ESettingsCategory::Renderer: DrawCategory_Renderer(); break;
					case ESettingsCategory::Camera:   DrawCategory_Camera(); break;
					case ESettingsCategory::Debug:    DrawCategory_Debug(State); break;
					default:                          break;
				}

				ImGui::PopStyleVar(2);
				UI::Font::Pop();
				State.bContentScrollable = (ImGui::GetScrollMaxY() > 1.0f);
				if (!State.bContentScrollable) {
					ImGui::SetScrollY(0.0f);
				}
				ImGui::EndChild();
				if (ScrollbarHidden) {
					ImGui::PopStyleColor(4);
				}
			}

			ImGui::EndTable();
		}
		ImGui::PopStyleVar(1);
	}

	void DrawSettingsDebugTools(FSettingsMenuState& State)
	{
		ImGuiStyle& Style = ImGui::GetStyle();
		if (State.bStyleEditor) {
			UI::FScopedFont Font(UI::Font::Get(EFont::SourceSansPro, EFontSize::Regular, EFontModifier::Normal));
			if (ImGui::Begin("Style Editor", &State.bStyleEditor)) {
				ImGui::ShowStyleEditor(&Style);
				ImGui::End();
			}
		}
		if (State.bIDStackTool) {
			ImGui::ShowIDStackToolWindow(&State.bIDStackTool);
		}
	}

}

