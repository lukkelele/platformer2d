#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/core.h"
#include "renderer/color.h"

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
