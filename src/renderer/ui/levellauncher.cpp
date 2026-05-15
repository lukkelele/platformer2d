#include "levellauncher.h"

#include "core/core.h"
#include "game/instance.h"
#include "renderer/color.h"
#include "renderer/ui/ui.h"
#include "renderer/ui/ui_core.h"
#include "renderer/ui/uilayer.h"

namespace platformer2d::UI {

	namespace {
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

		FNextLevel NextLevel{};
	}

	void TickLevelLauncher()
	{
		if (!NextLevel.bSelected) {
			return;
		}
		LK_ASSERT(CGameInstance::IsValid());
		LK_DEBUG_TAG("UILayer", "Level selected: {}", NextLevel.Path);
		CGameInstance::Get().OpenScene(NextLevel.Path);
		NextLevel.Clear();
	}

	void LevelLauncher()
	{
		const bool GameInstanceValid = CGameInstance::IsValid();
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

		/* @todo: Use single button func for levels */
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
					if (GameInstanceValid) {
						CGameInstance::Get().OpenScene(ScenePath);
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
					if (GameInstanceValid) {
						CGameInstance::Get().OpenScene(ScenePath);
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
				CUILayer::RequestMenu(CUILayer::EMenu::MainMenu);
			}
		}

		UI::Font::Pop();
		ImGui::PopStyleVar(2);

		if (!GameInstanceValid) {
			UI::EndViewport();
		}
	}

	bool MainMenuButton(const ImVec2& Size)
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

}

