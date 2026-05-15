#include "hudlayer.h"

#include "core/profiler.h"
#include "game/instance.h"
#include "game/player.h"
#include "renderer/ui/pausemenu.h"
#include "renderer/ui/ui.h"

namespace platformer2d {

	static void DrawHud(const std::shared_ptr<CPlayer>& Player);

	CHudLayer::CHudLayer(std::string_view InName)
		: CLayer(InName)
	{
	}

	void CHudLayer::Tick(const float DeltaTime)
	{
		LK_UNUSED(DeltaTime);
	}

	void CHudLayer::RenderUI()
	{
		LK_PROFILE_FUNC();
		if (!CGameInstance::IsValid()) {
			return;
		}

		auto& GameInstance = CGameInstance::Get();
		if (!GameInstance.HasScene()) {
			return;
		}

		if (UI::IsPauseMenuOpen()) {
			return;
		}

		if (std::shared_ptr<CPlayer> Player = GameInstance.GetPlayer(0)) {
			DrawHud(Player);
		}
	}

	void CHudLayer::OnAttach()
	{
		LK_DEBUG_TAG("HudLayer", "OnAttach");
	}

	void CHudLayer::OnDetach()
	{
		LK_DEBUG_TAG("HudLayer", "OnDetach");
	}

	static void DrawHud(const std::shared_ptr<CPlayer>& Player)
	{
		if (!Player) {
			return;
		}

		const ImGuiStyle& Style = ImGui::GetStyle();
		ImGuiViewport* Viewport = ImGui::GetMainViewport();
		constexpr ImVec2 WindowSize = ImVec2(390, 220);

		ImVec2 AnchorPos = Viewport->Pos;
		if (ImGuiWindow* ViewportPanel = ImGui::FindWindowByName(UI::PanelID::Viewport)) {
			AnchorPos = ViewportPanel->Pos;
		}

		const float PaddingX = Style.FramePadding.x + Style.DockingSeparatorSize + Style.ItemSpacing.y;
		float PaddingY = Style.FramePadding.x + Style.DockingSeparatorSize + Style.ItemSpacing.y;

		if (ImGuiWindow* BottomBar = ImGui::FindWindowByName(UI::PanelID::BottomBar)) {
			PaddingY += BottomBar->Size.y;
		}

		ImGui::SetNextWindowPos(ImVec2(AnchorPos.x + PaddingX, Viewport->Size.y - (WindowSize.y + PaddingY)), ImGuiCond_Always);
		ImGui::SetNextWindowSize(WindowSize, ImGuiCond_Always);
		ImGui::SetNextWindowBgAlpha(0.30f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
		constexpr ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs;
		const bool WindowOpen = UI::Begin("##PlayerHud", nullptr, WindowFlags);
		ImGui::PopStyleVar(1);
		if (!WindowOpen) {
			return;
		}

		constexpr float COLUMN_WIDTH = 210.0f;
		UI::BeginPropertyGrid(COLUMN_WIDTH);

		/* Health */
		{
			constexpr std::uint16_t HP = 100; /* @todo */
			ImGui::TableNextRow();
			UI::Table::Label("Health", EFont::SourceSansPro, EFontSize::Header, EFontModifier::Bold);

			UI::Table::NextColumn();
			UI::FScopedFont Font(EFont::SourceSansPro, EFontSize::Header, EFontModifier::Bold);
			std::uint32_t Color;
			if (HP > 50) {
				Color = RGBA32::LightGreen;
			} else if (HP > 25) {
				Color = RGBA32::Yellow;
			} else {
				Color = RGBA32::Red;
			}
			UI::FScopedColor TextColor(ImGuiCol_Text, Color);
			ImGui::Text("%d", HP);
		}

		/* Inventory info */
		{
			UI::Table::NextRow();
			UI::Table::Label("Inventory", EFont::SourceSansPro, EFontSize::Header, EFontModifier::Bold);
			UI::Table::NextColumn();
			const CInventory& Inventory = Player->GetInventory();
			const std::size_t UsedSlots = Inventory.GetUsedSlots();
			UI::FScopedFont Font(EFont::SourceSansPro, EFontSize::Header, EFontModifier::Bold);
			ImGui::Text("%d/%d", UsedSlots, CInventory::MAX_ITEMS);
		}

		/* Weapon */
		{
			std::shared_ptr<CRifle> Rifle = Player->GetRifle();
			UI::Table::NextRow();
			UI::Table::Label("Weapon", EFont::SourceSansPro, EFontSize::Header, EFontModifier::Bold);
			UI::Table::NextColumn();
			UI::FScopedFont Font(EFont::SourceSansPro, EFontSize::Header, EFontModifier::Bold);
			if (Rifle) {
				const bool Enabled = Rifle->IsEnabled();
				if (!Enabled) {
					ImGui::PushStyleColor(ImGuiCol_Text, RGBA32::Gray);
				}
				ImGui::Text("Rifle");
				if (!Enabled) {
					ImGui::PopStyleColor(1);
				}
			} else {
				ImGui::Text("None");
			}

			if (Rifle) {
				UI::Table::NextRow();
				UI::Table::Label("Ammo");
				UI::Table::NextColumn();
				const std::uint16_t Ammo = Rifle->GetAmmo();
				std::uint32_t Color = RGBA32::White;
				if (Ammo <= 3) {
					Color = RGBA32::Red;
				}
				UI::FScopedColor TextColor(ImGuiCol_Text, Color);
				ImGui::Text("%d", Ammo);
			}
		}

		UI::EndPropertyGrid();
		UI::End();
	}

}

