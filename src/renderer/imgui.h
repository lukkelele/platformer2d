#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace ImGui {

	inline bool TreeNodeBehaviorIsOpen(const ImGuiID NodeID, 
									   const ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_None)
	{
		ImGuiContext& G = *GImGui;
		ImGuiWindow* Window = G.CurrentWindow;
		const bool IsOpen = Window->StateStorage.GetInt(NodeID, (Flags & ImGuiTreeNodeFlags_DefaultOpen) ? 1 : 0) != 0;
		return IsOpen;
	}

}
