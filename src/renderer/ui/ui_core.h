#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <ImGuizmo/ImGuizmo.h>

#include "imgui.h"

#include "core/core.h"
#include "core/window.h"
#include "renderer/color.h"
#include "renderer/texture.h"
#include "scoped.h"

namespace platformer2d {
	class CActor;
}

namespace platformer2d::UI {

	namespace PanelID {
		inline constexpr const char* const CoreViewport = "CoreViewport";
		inline constexpr const char* const Viewport = "Viewport";
		inline constexpr const char* const HostWindow = "HostWindow";
		inline constexpr const char* const Dockspace = "##Dockspace";
		inline constexpr const char* const Topbar = "##Topbar";
		inline constexpr const char* const Sidebar1 = "Sidebar##1";
		inline constexpr const char* const Sidebar2 = "Sidebar##2";
		inline constexpr const char* const Menubar = "##Menubar";
		inline constexpr const char* const BottomBar = "##BottomBar";
		inline constexpr const char* const Selection = "Selection";
		inline constexpr const char* const SceneManager = "Scene Manager";
		inline constexpr const char* const Creator = "Creator";
		inline constexpr const char* const TerrainCreator = "Terrain Creator";
		inline constexpr const char* const TextureInspector = "Texture Inspector";
		inline constexpr const char* const SpriteInspector = "Sprite Inspector";
	}

	inline ImGuiWindowFlags CoreViewportFlags = ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoScrollbar
		| ImGuiWindowFlags_NoBringToFrontOnFocus
		| ImGuiWindowFlags_NoNavFocus
		| ImGuiWindowFlags_NoScrollWithMouse
		| ImGuiWindowFlags_NoInputs
		| ImGuiWindowFlags_NoBackground
		| ImGuiWindowFlags_NoDocking;

	inline ImGuiWindowFlags ViewportFlags = ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoScrollbar
		| ImGuiWindowFlags_NoScrollWithMouse
		| ImGuiWindowFlags_NoResize;

	inline constexpr ImGuiWindowFlags SidebarFlags = ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoBringToFrontOnFocus
		| ImGuiWindowFlags_NoNavFocus
		| ImGuiWindowFlags_NoMove;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
	inline constexpr ImGuiDockNodeFlags DockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode
		| ImGuiDockNodeFlags_NoWindowMenuButton;
#pragma GCC diagnostic pop

	void PushID();
	void PopID();
	const char* GenerateID();

	bool Begin(const char* WindowTitle, bool* Open = nullptr, ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_None);
	void End();

	enum class EDockLayout : std::uint8_t
	{
		Default,
		WideCanvas,
		COUNT
	};
	LK_ENUM(EDockLayout);

	void BeginCoreViewport();
	bool BeginViewport();
	void EndViewport();

	void SetDockLayout(EDockLayout Layout);
	void ResetDockLayout();
	EDockLayout GetDockLayout();

	void PrepareViewport();
	ImGuiDockNode* FindCentralNode(ImGuiID DockspaceID);
	void RouteToCentralNode();

	bool IsViewportTabActive();

	namespace Internal {
		struct FActorDataEntry
		{
			std::array<char, 64> NameBuf;
		};
	}

	struct FActorCache
	{
		std::unordered_map<LUUID, Internal::FActorDataEntry> Map;
		bool Contains(const LUUID Handle) const { return Map.contains(Handle); }
		bool Contains(const CActor& Actor) const;
		bool Cache(const CActor& Actor);

		auto Find(const LUUID Handle) { return Map.find(Handle); }
		auto End() const { return Map.end(); }
		auto Erase(const LUUID Handle) { return Map.erase(Handle); }
	};
	inline FActorCache ActorCache;

	void ShiftCursorX(float Distance);
	void ShiftCursorY(float Distance);
	void ShiftCursor(float X, float Y);
	bool IsItemHovered(float DelayInSeconds = 0.10f, ImGuiHoveredFlags Flags = ImGuiHoveredFlags_None);

	ImRect RectOffset(const ImRect& Rect, float X, float Y);
	ImRect GetItemRect();

	ImColor ColorWithMultipliedValue(const ImColor& Color, float Multiplier);
	ImColor ColorWithMultipliedSaturation(const ImColor& Color, float Multiplier);
	ImColor ColorWithMultipliedHue(const ImColor& Color, float Multiplier);

	void HelpMarker(const char* HelpDesc, const char* HelpSymbol = "(?)");
	void HoverText(const char* Text);
	void SetTooltip(std::string_view Text, float DelayInSeconds = 0.10f, bool AllowWhenDisabled = true, ImVec2 Padding = ImVec2(5, 5));

	inline void LargeText(std::string_view Text, const EFont Font = EFont::SourceSansPro, const EFontModifier Modifier = EFontModifier::Normal)
	{
		UI::FScopedFont ScopedFont(Font::Get(Font, EFontSize::Large, Modifier));
		ImGui::Text("%s", Text.data());
	}

	inline void LargeTextCentralized(std::string_view Text, const EFont Font = EFont::SourceSansPro, const EFontModifier Modifier = EFontModifier::Normal)
	{
		UI::FScopedFont ScopedFont(Font::Get(Font, EFontSize::Large, Modifier));
		const ImVec2 Size = ImGui::CalcTextSize(Text.data());
		const ImVec2 Avail = ImGui::GetContentRegionAvail();
		UI::ShiftCursorX((Avail.x * 0.50f) - (Size.x * 0.50f));
		ImGui::Text("%s", Text.data());
	}

	inline void HeaderText(std::string_view Text, const EFont Font = EFont::SourceSansPro, const EFontModifier Modifier = EFontModifier::Normal)
	{
		UI::FScopedFont ScopedFont(Font::Get(Font, EFontSize::Header, Modifier));
		ImGui::Text("%s", Text.data());
	}

	inline void HeaderTextCentralized(std::string_view Text, const EFont Font = EFont::SourceSansPro, const EFontModifier Modifier = EFontModifier::Normal)
	{
		UI::FScopedFont ScopedFont(Font::Get(Font, EFontSize::Header, Modifier));
		const ImVec2 Size = ImGui::CalcTextSize(Text.data());
		const ImVec2 Avail = ImGui::GetContentRegionAvail();
		UI::ShiftCursorX((Avail.x * 0.50f) - (Size.x * 0.50f));
		ImGui::Text("%s", Text.data());
	}

	inline void BannerText(std::string_view Text, const EFont Font = EFont::SourceSansPro, const EFontModifier Modifier = EFontModifier::Normal)
	{
		UI::FScopedFont ScopedFont(Font::Get(Font, EFontSize::Banner, Modifier));
		ImGui::Text("%s", Text.data());
	}

	inline void BannerTextCentralized(std::string_view Text, const EFont Font = EFont::SourceSansPro, const EFontModifier Modifier = EFontModifier::Normal)
	{
		UI::FScopedFont ScopedFont(Font::Get(Font, EFontSize::Banner, Modifier));
		const ImVec2 Size = ImGui::CalcTextSize(Text.data());
		const ImVec2 Avail = ImGui::GetContentRegionAvail();
		UI::ShiftCursorX((Avail.x * 0.50f) - (Size.x * 0.50f));
		ImGui::Text("%s", Text.data());
	}

	inline void Image(std::shared_ptr<CTexture> Texture, const ImVec2& Size, const ImVec2& UV0 = ImVec2(0, 1), const ImVec2& UV1 = ImVec2(1, 0))
	{
		ImGui::Image(static_cast<ImU64>(Texture->GetID()), Size, UV0, UV1);
	}

	inline void Separator(const float YPadding = 0.0f)
	{
		if (YPadding > 0.0f) {
			ImGui::Dummy(ImVec2(0, YPadding));
		}
		ImGui::Separator();
		if (YPadding > 0.0f) {
			ImGui::Dummy(ImVec2(0, YPadding));
		}
	}

	inline void VSplitter(const char* InID, float* InOutLeftWidth, const float Thickness, const float Height)
	{
		const ImVec2 Cursor = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton(InID, ImVec2(Thickness, Height));
		const bool Active = ImGui::IsItemActive();
		const bool Hovered = ImGui::IsItemHovered();
		if (Active) {
			*InOutLeftWidth += ImGui::GetIO().MouseDelta.x;
		}
		if (Active || Hovered) {
			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
		}

		const std::uint32_t Color = Active ? FColor::LightBlue.WithAlpha(0.90f).As<std::uint32_t>()
			: Hovered                      ? FColor::LightBlue.WithAlpha(0.70f).As<std::uint32_t>()
										   : FColor::Gray.WithAlpha(0.70f).As<std::uint32_t>();
		ImGui::GetWindowDrawList()->AddRectFilled(Cursor, ImVec2(Cursor.x + Thickness, Cursor.y + Height), Color);
	}

	inline void HSplitter(const char* InID, float* InOutTopHeight, const float Thickness, const float Width)
	{
		const ImVec2 Cursor = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton(InID, ImVec2(Width, Thickness));
		const bool Active = ImGui::IsItemActive();
		const bool Hovered = ImGui::IsItemHovered();
		if (Active) {
			*InOutTopHeight += ImGui::GetIO().MouseDelta.y;
		}
		if (Active || Hovered) {
			ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
		}

		const std::uint32_t Color = Active ? FColor::LightBlue.WithAlpha(0.90f).As<std::uint32_t>()
			: Hovered                      ? FColor::LightBlue.WithAlpha(0.70f).As<std::uint32_t>()
										   : FColor::Gray.WithAlpha(0.70f).As<std::uint32_t>();
		ImGui::GetWindowDrawList()->AddRectFilled(Cursor, ImVec2(Cursor.x + Width, Cursor.y + Thickness), Color);
	}

	/**
	 * @brief Get texture ID as an ImTextureID.
	 */
	inline ImTextureID GetTextureID(const std::shared_ptr<CTexture>& Texture)
	{
		return static_cast<ImTextureID>(Texture->GetID());
	};

	inline void DrawButtonImage(const std::shared_ptr<CTexture>& ImageNormal,
		const std::shared_ptr<CTexture>& ImageHovered,
		const std::shared_ptr<CTexture>& ImagePressed,
		const std::uint32_t TintNormal,
		const std::uint32_t TintHovered,
		const std::uint32_t TintPressed,
		const ImVec2& RectMin,
		const ImVec2& RectMax)
	{
		static constexpr ImVec2 UVMin(0, 0);
		static constexpr ImVec2 UVMax(1, 1);
		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		if (ImGui::IsItemActive()) {
			DrawList->AddImage(GetTextureID(ImagePressed), RectMin, RectMax, UVMin, UVMax, TintPressed);
		} else if (ImGui::IsItemHovered()) {
			DrawList->AddImage(GetTextureID(ImageHovered), RectMin, RectMax, UVMin, UVMax, TintHovered);
		} else {
			DrawList->AddImage(GetTextureID(ImageNormal), RectMin, RectMax, UVMin, UVMax, TintNormal);
		}
	}

	inline void DrawButtonImage(const std::shared_ptr<CTexture>& Image,
		const std::uint32_t TintNormal,
		const std::uint32_t TintHovered,
		const std::uint32_t TintPressed,
		const ImRect& Rectangle)
	{
		DrawButtonImage(Image, Image, Image, TintNormal, TintHovered, TintPressed, Rectangle.Min, Rectangle.Max);
	}

	inline void Underline(bool FullWidth = false, const float OffsetX = 0.0f, const float OffsetY = -1.0f)
	{
		if (FullWidth) {
			if (ImGui::GetCurrentWindow()->DC.CurrentColumns != nullptr) {
				ImGui::PushColumnsBackground();
			} else if (ImGui::GetCurrentTable() != nullptr) {
				ImGui::TablePushBackgroundChannel();
			}
		}

		const float Width = FullWidth ? ImGui::GetWindowWidth() : ImGui::GetContentRegionAvail().x;
		const ImVec2 Cursor = ImGui::GetCursorScreenPos();
		ImGui::GetWindowDrawList()->AddLine(
			ImVec2(Cursor.x + OffsetX, Cursor.y + OffsetY),
			ImVec2(Cursor.x + Width, Cursor.y + OffsetY),
			RGBA32::BackgroundDark,
			1.0f);

		if (FullWidth) {
			if (ImGui::GetCurrentWindow()->DC.CurrentColumns != nullptr) {
				ImGui::PopColumnsBackground();
			} else if (ImGui::GetCurrentTable() != nullptr) {
				ImGui::TablePopBackgroundChannel();
			}
		}
	}
}
