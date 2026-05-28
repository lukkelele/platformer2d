#pragma once

#include <span>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/core.h"
#include "renderer/texture.h"
#include "scoped.h"

namespace platformer2d::UI {

	template<typename TEnum>
	inline bool Combo(std::string_view Label, std::span<const TEnum> Options, TEnum& Selected)
	{
		static const auto EnumNames = Enum::View<TEnum, const char*>();
		const auto SelectedIdx = std::to_underlying(Selected);

		std::array<char, 64> NameBuf = {0};
		std::snprintf(NameBuf.data(), NameBuf.size(), "%s", Label.data());

		UI::FScopedStyle FrameRounding(ImGuiStyleVar_FrameRounding, 6.0f);
		UI::FScopedStyle PopupRounding(ImGuiStyleVar_PopupRounding, 6.0f);
		if (!ImGui::BeginCombo(NameBuf.data(), EnumNames[SelectedIdx])) {
			return false;
		}

		bool Updated = false;
		for (std::size_t Idx = 0; Idx < Options.size(); Idx++) {
			const bool IsSelected = (SelectedIdx == Idx);
			if (ImGui::Selectable(EnumNames[Idx], IsSelected)) {
				if (SelectedIdx != Idx) {
					Updated = true;
					Selected = static_cast<TEnum>(Idx);
				}
			}
		}

		ImGui::EndCombo();
		return Updated;
	}

	void TextureModifier();
	bool TextureDropdown(ETexture& Selected);
	bool BlendFunction(float IndentX = 0.0f);
	bool DepthFunction(float IndentX = 0.0f);

}

