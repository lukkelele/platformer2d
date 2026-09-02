#include "hudlayer.h"

#include "core/profiler.h"
#include "core/settings.h"
#include "core/input/keyboard.h"
#include "game/instance.h"
#include "game/player.h"
#include "game/rifle.h"
#include "renderer/color.h"
#include "renderer/fontawesome.h"
#include "renderer/font.h"
#include "renderer/ui/pausemenu.h"
#include "renderer/ui/scoped.h"
#include "renderer/ui/ui.h"
#include "scene/components.h"

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
		LK_PROFILER_SCOPED();
		if (bInputDebug) {
			UI::InputDebug();
		}

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

		if (!UI::IsViewportTabActive()) {
			return;
		}

		if (std::shared_ptr<CPlayer> Player = GameInstance.GetPlayer(0)) {
			DrawHud(Player);
		}
	}

	void CHudLayer::OnAttach()
	{
		LK_DEBUG_TAG("HudLayer", "OnAttach");
		OnKeyHandle = CKeyboard::OnKeyEvent.Add(this, &CHudLayer::OnKey);
	}

	void CHudLayer::OnDetach()
	{
		LK_DEBUG_TAG("HudLayer", "OnDetach");
		CKeyboard::OnKeyEvent.Remove(OnKeyHandle);
	}

	void CHudLayer::OnKey(const FKeyData& Data)
	{
		switch (Data.Key) {
			case EKey::F1:
				if (Data.State == EKeyState::Pressed) {
					SetInputDebug(!IsInputDebugEnabled());
					LK_DEBUG_TAG("HudLayer", "Input debug: {}", bInputDebug ? "ON" : "OFF");
				}
				break;

			default: break;
		}
	}

	static void GetPlayerHealth(const std::shared_ptr<CPlayer>& Player, float& OutHealth, float& OutMax)
	{
		OutHealth = 100.0f;
		OutMax = 100.0f;
		if (Player->HasComponent<FHealthComponent>()) {
			const FHealthComponent& HC = Player->GetComponent<FHealthComponent>();
			OutHealth = HC.GetHealth();
			OutMax = (HC.MaxHealth > 0.0f) ? HC.MaxHealth : 100.0f;
		}
	}

	static std::uint32_t HealthColor(const float Ratio)
	{
		if (Ratio > 0.50f) {
			return IM_COL32(75, 200, 95, 255);
		}
		if (Ratio > 0.25f) {
			return IM_COL32(235, 195, 60, 255);
		}
		return IM_COL32(225, 70, 65, 255);
	}

	static void DrawHealthBar(const float Ratio, const ImVec2& Pos, const ImVec2& Size, const float HealthValue, const float MaxHealth)
	{
		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		const float Rounding = Size.y * 0.50f;

		DrawList->AddRectFilled(Pos, ImVec2(Pos.x + Size.x, Pos.y + Size.y), IM_COL32(20, 22, 26, 220), Rounding);

		const float FillWidth = Size.x * Ratio;
		if (FillWidth > 1.0f) {
			DrawList->AddRectFilled(Pos,
				ImVec2(Pos.x + FillWidth, Pos.y + Size.y),
				HealthColor(Ratio),
				Rounding);
		}
		DrawList->AddRect(Pos, ImVec2(Pos.x + Size.x, Pos.y + Size.y), IM_COL32(255, 255, 255, 45), Rounding, 0, 1.5f);

		std::array<char, 32> Label{};
		std::snprintf(Label.data(), Label.size(), "%d / %d", static_cast<int>(HealthValue), static_cast<int>(MaxHealth));
		const ImVec2 TextSize = ImGui::CalcTextSize(Label.data());
		const ImVec2 TextPos = ImVec2(
			Pos.x + (Size.x - TextSize.x) * 0.50f,
			Pos.y + (Size.y - TextSize.y) * 0.50f);
		DrawList->AddText(ImVec2(TextPos.x + 1.0f, TextPos.y + 1.0f), IM_COL32(0, 0, 0, 200), Label.data());
		DrawList->AddText(TextPos, IM_COL32(255, 255, 255, 230), Label.data());
	}

	static void DrawAmmoPill(const std::shared_ptr<CRifle>& Rifle, const ImVec2& Pos, const ImVec2& Size)
	{
		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		const float Rounding = Size.y * 0.50f;

		const std::uint16_t Ammo = Rifle->GetAmmo();
		const std::uint16_t MaxAmmo = Rifle->GetMagazineSize();
		const bool LowAmmo = (Ammo <= 3);
		const bool Enabled = Rifle->IsEnabled();

		const std::uint32_t BgColor = Enabled ? IM_COL32(20, 22, 26, 220) : IM_COL32(40, 40, 42, 200);
		const std::uint32_t AccentColor = LowAmmo ? IM_COL32(225, 70, 65, 255)
			: Enabled                             ? IM_COL32(80, 165, 220, 255)
												  : IM_COL32(110, 110, 115, 255);

		DrawList->AddRectFilled(Pos, ImVec2(Pos.x + Size.x, Pos.y + Size.y), BgColor, Rounding);
		DrawList->AddRect(Pos, ImVec2(Pos.x + Size.x, Pos.y + Size.y), IM_COL32(255, 255, 255, 45), Rounding, 0, 1.5f);

		const float IconBoxWidth = Size.y;
		const ImVec2 IconCenter = ImVec2(Pos.x + IconBoxWidth * 0.50f, Pos.y + Size.y * 0.50f);
		const ImVec2 IconSize = ImGui::CalcTextSize(LK_ICON_CROSSHAIRS);
		DrawList->AddText(ImVec2(IconCenter.x - IconSize.x * 0.50f, IconCenter.y - IconSize.y * 0.50f),
			AccentColor, LK_ICON_CROSSHAIRS);

		std::array<char, 32> Label{};
		std::snprintf(Label.data(), Label.size(), "%u / %u", Ammo, MaxAmmo);

		const ImVec2 TextSize = ImGui::CalcTextSize(Label.data());
		const ImVec2 TextPos = ImVec2(Pos.x + IconBoxWidth + (Size.x - IconBoxWidth - TextSize.x) * 0.50f,
			Pos.y + (Size.y - TextSize.y) * 0.50f);
		DrawList->AddText(TextPos, IM_COL32(245, 245, 245, 230), Label.data());
	}

	static void DrawInventoryChip(const std::shared_ptr<CPlayer>& Player, const ImVec2& Pos, const ImVec2& Size)
	{
		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		const float Rounding = Size.y * 0.30f;

		const CInventory& Inventory = Player->GetInventory();
		const std::size_t Used = Inventory.GetUsedSlots();
		const std::size_t Max = CInventory::MAX_ITEMS;

		DrawList->AddRectFilled(Pos, ImVec2(Pos.x + Size.x, Pos.y + Size.y), IM_COL32(20, 22, 26, 200), Rounding);
		DrawList->AddRect(Pos, ImVec2(Pos.x + Size.x, Pos.y + Size.y),
			IM_COL32(255, 255, 255, 35), Rounding, 0, 1.0f);

		const ImVec2 IconSize = ImGui::CalcTextSize(LK_ICON_BRIEFCASE);
		const float Inner = 8.0f;
		const ImVec2 IconPos = ImVec2(Pos.x + Inner, Pos.y + (Size.y - IconSize.y) * 0.50f);
		DrawList->AddText(IconPos, IM_COL32(200, 175, 95, 230), LK_ICON_BRIEFCASE);

		std::array<char, 32> Label{};
		std::snprintf(Label.data(), Label.size(), "%zu / %zu", Used, Max);
		const ImVec2 TextSize = ImGui::CalcTextSize(Label.data());
		const ImVec2 TextPos = ImVec2(IconPos.x + IconSize.x + Inner, Pos.y + (Size.y - TextSize.y) * 0.50f);
		DrawList->AddText(TextPos, IM_COL32(220, 220, 220, 220), Label.data());
	}

	static void DrawHud(const std::shared_ptr<CPlayer>& Player)
	{
		if (!Player) {
			return;
		}

		const ImGuiStyle& Style = ImGui::GetStyle();
		ImGuiViewport* Viewport = ImGui::GetMainViewport();
		const float UIScale = std::clamp(FSettings::Get().Graphics.UIScale, 0.50f, 3.0f);

		const float HealthBarHeight = 36.0f * UIScale;
		const float HealthBarWidth = 240.0f * UIScale;
		const float AmmoPillWidth = 120.0f * UIScale;
		const float InventoryChipWidth = 90.0f * UIScale;
		const float Gap = 14.0f * UIScale;

		const float ContentWidth = HealthBarWidth;
		const float ContentHeight = HealthBarHeight + Gap + HealthBarHeight;
		const float Padding = 16.0f * UIScale;
		const ImVec2 WindowSize = ImVec2(ContentWidth + Padding * 2.0f, ContentHeight + Padding * 2.0f);

		ImVec2 AnchorPos = Viewport->Pos;
		if (ImGuiWindow* ViewportPanel = ImGui::FindWindowByName(UI::PanelID::Viewport)) {
			AnchorPos = ViewportPanel->Pos;
		}

		const float PaddingX = Style.FramePadding.x + Style.DockingSeparatorSize + Style.ItemSpacing.y;
		float PaddingY = Style.FramePadding.x + Style.DockingSeparatorSize + Style.ItemSpacing.y;
		if (ImGuiWindow* BottomBar = ImGui::FindWindowByName(UI::PanelID::BottomBar)) {
			PaddingY += BottomBar->Size.y;
		}

		ImGui::SetNextWindowPos(ImVec2(AnchorPos.x + PaddingX,
									Viewport->Size.y - (WindowSize.y + PaddingY)),
			ImGuiCond_Always);
		ImGui::SetNextWindowSize(WindowSize, ImGuiCond_Always);
		ImGui::SetNextWindowBgAlpha(0.35f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(Padding, Padding));
		constexpr ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs;
		const bool WindowOpen = UI::Begin("##PlayerHud", nullptr, WindowFlags);
		ImGui::PopStyleVar(2);
		if (!WindowOpen) {
			return;
		}

		const ImVec2 Origin = ImGui::GetCursorScreenPos();

		/* Row 1: Health bar */
		{
			UI::FScopedFont IconFont(EFont::SourceSansPro, EFontSize::Header, EFontModifier::Bold);
			const ImVec2 IconSize = ImGui::CalcTextSize(LK_ICON_HEART);
			const float IconWidth = std::max(IconSize.x, 22.0f * UIScale);

			float Health = 100.0f;
			float MaxHealth = 100.0f;
			GetPlayerHealth(Player, Health, MaxHealth);
			const float Ratio = std::clamp(Health / MaxHealth, 0.0f, 1.0f);

			ImDrawList* DrawList = ImGui::GetWindowDrawList();
			const ImVec2 IconPos = ImVec2(Origin.x, Origin.y + (HealthBarHeight - IconSize.y) * 0.50f);
			DrawList->AddText(IconPos, HealthColor(Ratio), LK_ICON_HEART);

			const ImVec2 BarPos = ImVec2(Origin.x + IconWidth + Gap, Origin.y);
			const ImVec2 BarSize = ImVec2(HealthBarWidth - IconWidth - Gap, HealthBarHeight);
			DrawHealthBar(Ratio, BarPos, BarSize, Health, MaxHealth);
		}

		/* Row 2: Inventory + Ammo */
		{
			const float Row2Y = Origin.y + HealthBarHeight + Gap;
			const ImVec2 InvPos = ImVec2(Origin.x + AmmoPillWidth + Gap, Row2Y);
			const ImVec2 InvSize = ImVec2(InventoryChipWidth, HealthBarHeight);
			DrawInventoryChip(Player, InvPos, InvSize);

			if (std::shared_ptr<CRifle> Rifle = Player->GetRifle()) {
				const ImVec2 AmmoPos = ImVec2(Origin.x, Row2Y);
				const ImVec2 PillSize = ImVec2(AmmoPillWidth, HealthBarHeight);
				DrawAmmoPill(Rifle, AmmoPos, PillSize);
			}
		}

		ImGui::Dummy(ImVec2(ContentWidth, ContentHeight));
		UI::End();
	}

}
