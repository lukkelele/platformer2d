#pragma once

#include "core/core.h"
#include "core/log_formatters.h"

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

template<>
struct lklog::fmt::formatter<ImVec2>
{
	template<typename ParseContext>
	constexpr auto parse(ParseContext& Context)
	{
		return Context.begin();
	}

	template<typename FormatContext>
	auto format(const ImVec2& Vec, FormatContext& Context) const
	{
		return lklog::fmt::format_to(Context.out(), "({:.2f}, {:.2f})", Vec.x, Vec.y);
	}
};

template<>
struct lklog::fmt::formatter<ImVec4>
{
	template<typename ParseContext>
	constexpr auto parse(ParseContext& Context)
	{
		return Context.begin();
	}

	template<typename FormatContext>
	auto format(const ImVec4& Vec, FormatContext& Context) const
	{
		return lklog::fmt::format_to(Context.out(), "({:.2f}, {:.2f}, {:.2f}, {:.2f})", Vec.x, Vec.y, Vec.z, Vec.w);
	}
};
