#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/core.h"
#include "renderer/color.h"
#include "scoped.h"

namespace platformer2d::UI {

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

	inline bool IsItemHovered(const float DelayInSeconds = 0.10f, ImGuiHoveredFlags Flags = ImGuiHoveredFlags_None)
	{
		return ImGui::IsItemHovered() && (GImGui->HoveredIdTimer > DelayInSeconds); /* HoveredIdNotActiveTimer. */
	}

	inline void HelpMarker(const char* HelpDesc, const char* HelpSymbol = "(?)")
	{
		static constexpr float WrapPosOffset = 35.0f;
		ImGui::TextDisabled(HelpSymbol);
		if (ImGui::IsItemHovered())
		{
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
		if (ImGui::IsItemHovered())
		{
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
		if (IsItemHovered(DelayInSeconds, AllowWhenDisabled ? ImGuiHoveredFlags_AllowWhenDisabled : ImGuiHoveredFlags_None))
		{
			UI::FScopedStyle WindowPadding(ImGuiStyleVar_WindowPadding, Padding);
			UI::FScopedColor TextColor(ImGuiCol_Text, RGBA32::Text::Brighter);
			ImGui::SetTooltip(Text.data());
		}
	}

	namespace Draw 
	{
		inline void Underline(bool FullWidth = false, const float OffsetX = 0.0f, const float OffsetY = -1.0f)
		{
			if (FullWidth)
			{
				if (ImGui::GetCurrentWindow()->DC.CurrentColumns != nullptr)
				{
					ImGui::PushColumnsBackground();
				}
				else if (ImGui::GetCurrentTable() != nullptr)
				{
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

			if (FullWidth)
			{
				if (ImGui::GetCurrentWindow()->DC.CurrentColumns != nullptr)
				{
					ImGui::PopColumnsBackground();
				}
				else if (ImGui::GetCurrentTable() != nullptr)
				{
					ImGui::TablePopBackgroundChannel();
				}
			}
		}
	}

}
