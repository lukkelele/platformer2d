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

namespace platformer2d::UI {

	namespace PanelID {
		inline constexpr const char* const CoreViewport = "##CoreViewport";
		inline constexpr const char* const EditorViewport = "##EditorViewport";
		inline constexpr const char* const Dockspace = "##Dockspace";
		inline constexpr const char* const HostWindow = "##HostWindow";
		inline constexpr const char* const Topbar = "##Topbar";
		inline constexpr const char* const Sidebar1 = "##Sidebar1";
		inline constexpr const char* const Sidebar2 = "##Sidebar2";
		inline constexpr const char* const Selection = "Selection";
		inline constexpr const char* const SceneManager = "##SceneManager";
		inline constexpr const char* const ContentBrowser = "##ContentBrowser";
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
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoBackground
		| ImGuiWindowFlags_NoBringToFrontOnFocus
		| ImGuiWindowFlags_NoDocking;

	inline constexpr ImGuiWindowFlags EditorViewportFlags = ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoScrollbar
		| ImGuiWindowFlags_NoScrollWithMouse
		| ImGuiWindowFlags_NoResize;

	inline ImGuiWindowFlags HostWindowFlags = ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_NoNavFocus
		| ImGuiWindowFlags_NoInputs
		| ImGuiWindowFlags_NoBringToFrontOnFocus
		| ImGuiWindowFlags_NoBackground
		| ImGuiWindowFlags_NoSavedSettings;

	inline constexpr ImGuiWindowFlags SidebarFlags = ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoCollapse
		| ImGuiWindowFlags_NoBringToFrontOnFocus
		| ImGuiWindowFlags_NoNavFocus
		| ImGuiWindowFlags_NoMove;

	inline constexpr ImGuiDockNodeFlags DockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode
		| ImGuiDockNodeFlags_NoDockingInCentralNode;

	struct FViewportData
	{
		glm::vec2 MenuBarSize = { 0.0f, 30.0f };
		glm::vec2 LeftSidebarSize = { 340.0f, 0.0f };
		glm::vec2 RightSidebarSize = { 340.0f, 0.0f };
	};
	extern FViewportData ViewportData;
	const FViewportData& GetViewportData();

	void PushID();
	void PopID();
	const char* GenerateID();

	bool Begin(const char* WindowTitle, bool* Open = nullptr, ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_None);
	void End();

	void BeginViewport(CWindow* Window);
	ImGuiDockNode* FindCentralNode(ImGuiID DockspaceID);

	inline void ShiftCursorX(const float Distance)
	{
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + Distance);
	}

	inline void ShiftCursorY(const float Distance)
	{
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Distance);
	}

	inline void ShiftCursor(const float InX, const float InY)
	{
		const ImVec2 Cursor = ImGui::GetCursorPos();
		ImGui::SetCursorPos(ImVec2(Cursor.x + InX, Cursor.y + InY));
	}

	inline bool IsItemHovered(const float DelayInSeconds = 0.10f, ImGuiHoveredFlags Flags = ImGuiHoveredFlags_None)
	{
		return ImGui::IsItemHovered() && (GImGui->HoveredIdTimer > DelayInSeconds); /* HoveredIdNotActiveTimer. */
	}

	inline ImRect RectOffset(const ImRect& Rect, const float X, const float Y)
	{
		ImRect Result = Rect;
		Result.Min.x += X;
		Result.Min.y += Y;
		Result.Max.x += X;
		Result.Max.y += Y;
		return Result;
	}

	inline ImRect GetItemRect()
	{
		return ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
	}

	inline ImColor ColorWithMultipliedValue(const ImColor& Color, const float Multiplier)
	{
		const ImVec4& ColorRaw = Color.Value;
		float Hue, Saturation, Value;
		ImGui::ColorConvertRGBtoHSV(ColorRaw.x, ColorRaw.y, ColorRaw.z, Hue, Saturation, Value);
		return ImColor::HSV(Hue, Saturation, std::min(Value * Multiplier, 1.0f));
	}

	inline ImColor ColorWithMultipliedSaturation(const ImColor& Color, const float Multiplier)
	{
		const ImVec4& ColorRaw = Color.Value;
		float Hue, Saturation, Value;
		ImGui::ColorConvertRGBtoHSV(ColorRaw.x, ColorRaw.y, ColorRaw.z, Hue, Saturation, Value);
		return ImColor::HSV(Hue, std::min(Saturation * Multiplier, 1.0f), Value);
	}

	inline ImColor ColorWithMultipliedHue(const ImColor& Color, const float Multiplier)
	{
		const ImVec4& ColorRaw = Color.Value;
		float Hue, Saturation, Value;
		ImGui::ColorConvertRGBtoHSV(ColorRaw.x, ColorRaw.y, ColorRaw.z, Hue, Saturation, Value);
		return ImColor::HSV(std::min(Hue * Multiplier, 1.0f), Saturation, Value);
	}

	inline void HelpMarker(const char* HelpDesc, const char* HelpSymbol = "(?)")
	{
		static constexpr float WrapPosOffset = 35.0f;
		ImGui::TextDisabled(HelpSymbol);
		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * WrapPosOffset);
			ImGui::TextUnformatted(HelpDesc);
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}

	inline void HoverText(const char* Text)
	{
		static constexpr float WrapPosOffset = 35.0f;
		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * WrapPosOffset);
			ImGui::TextUnformatted(Text);
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}

	inline void SetTooltip(std::string_view Text, const float DelayInSeconds = 0.10f,
						   const bool AllowWhenDisabled = true, const ImVec2 Padding = ImVec2(5, 5))
	{
		if (IsItemHovered(DelayInSeconds, AllowWhenDisabled ? ImGuiHoveredFlags_AllowWhenDisabled : ImGuiHoveredFlags_None)) {
			UI::FScopedStyle WindowPadding(ImGuiStyleVar_WindowPadding, Padding);
			UI::FScopedColor TextColor(ImGuiCol_Text, RGBA32::Text::Brighter);
			ImGui::SetTooltip(Text.data());
		}
	}

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

	inline void SeparatorPadded(const float YPadding)
	{
		ImGui::Dummy(ImVec2(0, YPadding));
		ImGui::Separator();
		ImGui::Dummy(ImVec2(0, YPadding));
	}

	/**
	 * @brief Get texture ID as an ImTextureID.
	 */
	FORCEINLINE ImTextureID GetTextureID(const std::shared_ptr<CTexture>& Texture)
	{
		return static_cast<ImTextureID>(Texture->GetID());
	};

	inline void DrawButtonImage(const std::shared_ptr<CTexture>& ImageNormal,
								const std::shared_ptr<CTexture>& ImageHovered,
								const std::shared_ptr<CTexture>& ImagePressed,
								const ImU32 TintNormal,
								const ImU32 TintHovered,
								const ImU32 TintPressed,
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
								const ImU32 TintNormal,
								const ImU32 TintHovered,
								const ImU32 TintPressed,
								const ImRect& Rectangle)
	{
		DrawButtonImage(Image, Image, Image, TintNormal, TintHovered, TintPressed, Rectangle.Min, Rectangle.Max);
	}

	namespace Widget {

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
				1.0f
			);

			if (FullWidth) {
				if (ImGui::GetCurrentWindow()->DC.CurrentColumns != nullptr) {
					ImGui::PopColumnsBackground();
				} else if (ImGui::GetCurrentTable() != nullptr) {
					ImGui::TablePopBackgroundChannel();
				}
			}
		}

	}
}

