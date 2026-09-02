#include "levellauncher.h"

#include <array>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>

#include "core/core.h"
#include "core/profiler.h"
#include "game/checkpointsystem.h"
#include "game/instance.h"
#include "renderer/color.h"
#include "renderer/font.h"
#include "renderer/renderer.h"
#include "renderer/texture.h"
#include "renderer/ui/scoped.h"
#include "renderer/ui/ui.h"
#include "renderer/ui/ui_core.h"
#include "renderer/ui/uilayer.h"

namespace platformer2d::UI {

	namespace {
		struct FNextLevel
		{
			bool bSelected = false;
			std::filesystem::path Path{};
			Core::ELayer Layer = Core::ELayer::Runtime;

			void Clear()
			{
				bSelected = false;
				Path.clear();
				Layer = Core::ELayer::Runtime;
			}
		};

		FNextLevel NextLevel{};

		struct FLevelEntry
		{
			const char* Name;
			const char* ScenePath;
			ETexture Texture;
			std::uint32_t AccentColor;
			Core::ELayer LaunchLayer;
		};

		static const std::array<FLevelEntry, 2> LevelEntries = {
			{
             {"Lukkelele's World", SCENES_DIR "/LukkelelesWorld.lscene", ETexture::Rifle, RGBA32::SmoothGreen, Core::ELayer::Runtime},
             {"Test Level", SCENES_DIR "/TestLevel.lscene", ETexture::Bricks, RGBA32::DarkGray, Core::ELayer::Editor},
			 }
        };
	}

	static bool DrawLevelCard(const FLevelEntry& Entry, const FCheckpointPreview& Preview, const ImVec2& Size, const std::size_t Index)
	{
		ImGui::PushID(static_cast<int>(Index));
		const ImVec2 CardPos = ImGui::GetCursorScreenPos();
		const bool Clicked = ImGui::InvisibleButton("##card", Size);
		const bool Hovered = ImGui::IsItemHovered();
		const bool Active = ImGui::IsItemActive();

		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		const ImVec2 CardMin = CardPos;
		const ImVec2 CardMax(CardPos.x + Size.x, CardPos.y + Size.y);

		constexpr float Rounding = 10.0f;
		const std::uint32_t BgIdle = FColor::Convert<std::uint32_t>(glm::vec4(0.10f, 0.10f, 0.13f, 1.0f));
		const std::uint32_t BgHover = FColor::Convert<std::uint32_t>(glm::vec4(0.16f, 0.16f, 0.20f, 1.0f));
		const std::uint32_t BgActive = FColor::Convert<std::uint32_t>(glm::vec4(0.22f, 0.22f, 0.26f, 1.0f));
		const std::uint32_t BgColor = Active ? BgActive : (Hovered ? BgHover : BgIdle);
		const std::uint32_t BorderColor = Hovered ? Entry.AccentColor : RGBA32::Muted;
		const float BorderThickness = Hovered ? 2.0f : 1.0f;

		DrawList->AddRectFilled(CardMin, CardMax, BgColor, Rounding);
		DrawList->AddRect(CardMin, CardMax, BorderColor, Rounding, 0, BorderThickness);

		constexpr float ImgPad = 10.0f;
		const float ImgH = Size.y * 0.52f;
		const ImVec2 ImgMin(CardMin.x + ImgPad, CardMin.y + ImgPad);
		const ImVec2 ImgMax(CardMax.x - ImgPad, CardMin.y + ImgH);

		std::shared_ptr<CTexture> Tex = CRenderer::GetTexture(Entry.Texture);
		if (Tex && (Tex->GetWidth() > 0) && (Tex->GetHeight() > 0)) {
			DrawList->AddImageRounded(static_cast<ImU64>(Tex->GetID()),
				ImgMin, ImgMax, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f),
				RGBA32::White, Rounding * 0.60f);
		} else {
			DrawList->AddRectFilled(ImgMin, ImgMax, RGBA32::DarkerGray, Rounding * 0.6f);
		}

		DrawList->AddRect(ImgMin, ImgMax, RGBA32::Muted, Rounding * 0.6f, 0, 1.0f);

		const float TextX = CardMin.x + 18.0f;
		float TextY = ImgMax.y + 16.0f;

		{
			UI::FScopedFont NameFont(EFont::Roboto, EFontSize::Larger, EFontModifier::Bold);
			DrawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(TextX, TextY), RGBA32::White, Entry.Name);
			TextY += ImGui::GetFontSize() + 12.0f;
		}

		std::array<char, 128> StatusBuf = {0};
		std::uint32_t StatusColor = RGBA32::Text::Darker;
		if (Preview.bExists) {
			std::snprintf(StatusBuf.data(), StatusBuf.size(), LK_ICON_FLAG "  In progress: %s", Preview.CurrentID.c_str());
			StatusColor = RGBA32::NiceGreen;
		} else {
			std::snprintf(StatusBuf.data(), StatusBuf.size(), LK_ICON_HOURGLASS "  Not started");
		}

		std::array<char, 64> ProgressBuf = {0};
		std::snprintf(ProgressBuf.data(), ProgressBuf.size(), LK_ICON_CHECK_CIRCLE_O "  Checkpoints cleared: %zu", Preview.TriggeredCount);
		{
			UI::FScopedFont InfoFont(EFont::Roboto, EFontSize::Large, EFontModifier::Normal);
			DrawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(TextX, TextY), StatusColor, StatusBuf.data());
			TextY += ImGui::GetFontSize() + 6.0f;
			DrawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), ImVec2(TextX, TextY), RGBA32::Text::Darker, ProgressBuf.data());
		}

		ImGui::PopID();
		return Clicked;
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
		LK_PROFILER_SCOPED();
		const bool GameInstanceValid = CGameInstance::IsValid();
		if (!GameInstanceValid) {
			UI::BeginViewport();
		}

		UI::Font::Push(EFont::Roboto, EFontSize::Header, EFontModifier::Bold);

		const ImVec2 Avail = ImGui::GetContentRegionAvail();
		UI::ShiftCursorY(Avail.y * 0.04f);
		UI::BannerTextCentralized("Levels", EFont::SourceSansPro, EFontModifier::Bold);
		ImGui::Dummy(ImVec2(0, 24));

		constexpr ImVec2 CardSize(360.0f, 440.0f);
		constexpr float CardSpacingX = 32.0f;
		constexpr float CardSpacingY = 32.0f;

		const float ContentWidth = ImGui::GetContentRegionAvail().x;
		std::size_t CardsPerRow = static_cast<std::size_t>((ContentWidth + CardSpacingX) / (CardSize.x + CardSpacingX));
		if (CardsPerRow == 0) {
			CardsPerRow = 1;
		}

		const float StartCursorY = ImGui::GetCursorPosY();
		float CurrentRowY = StartCursorY;

		std::array<FCheckpointPreview, LevelEntries.size()> Previews;
		for (std::size_t Idx = 0; Idx < LevelEntries.size(); Idx++) {
			Previews[Idx] = CCheckpointSystem::PeekFromDisk(LevelEntries[Idx].ScenePath);
		}

		for (std::size_t Idx = 0; Idx < LevelEntries.size(); Idx++) {
			const std::size_t RowIdx = Idx / CardsPerRow;
			const std::size_t ColIdx = Idx % CardsPerRow;

			if (ColIdx == 0) {
				if (Idx > 0) {
					CurrentRowY += CardSize.y + CardSpacingY;
				}
				const std::size_t Remaining = LevelEntries.size() - RowIdx * CardsPerRow;
				const std::size_t CardsInThisRow = (Remaining < CardsPerRow) ? Remaining : CardsPerRow;
				const float RowWidth = CardsInThisRow * CardSize.x + (CardsInThisRow - 1) * CardSpacingX;
				const float RowStartX = (ContentWidth - RowWidth) * 0.5f;
				ImGui::SetCursorPos(ImVec2(RowStartX < 0.0f ? 0.0f : RowStartX, CurrentRowY));
			} else {
				ImGui::SameLine(0.0f, CardSpacingX);
			}

			if (DrawLevelCard(LevelEntries[Idx], Previews[Idx], CardSize, Idx)) {
				const std::filesystem::path ScenePath(LevelEntries[Idx].ScenePath);
				if (GameInstanceValid) {
					CGameInstance::Get().OpenScene(ScenePath);
				} else {
					Core::Global.AddLayer(LevelEntries[Idx].LaunchLayer);
					NextLevel.bSelected = true;
					NextLevel.Path = ScenePath;
					NextLevel.Layer = LevelEntries[Idx].LaunchLayer;
				}
			}
		}

		if (UI::MainMenuButton(ImVec2(392.0f, 72.0f))) {
			LK_TRACE_TAG("UI", "Enter main menu");
			CUILayer::RequestMenu(CUILayer::EMenu::MainMenu);
		}

		UI::Font::Pop();

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
