#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/core.h"
#include "renderer/texture.h"

namespace platformer2d::UI {

	template<std::size_t N, typename TEnum>
	inline bool Combo(std::string_view Label, const std::array<TEnum, N>& Options, TEnum& Selected)
	{
		char NameBuf[64] = { 0 };
		std::snprintf(NameBuf, sizeof(NameBuf), "%s", Label.data());
		if (!ImGui::BeginCombo(NameBuf, Enum::ToString(Selected))) {
			return false;
		}

		bool Updated = false;
		for (std::size_t Idx = 0; Idx < Options.size(); Idx++) {
			const char* Option = Enum::ToString(Options[Idx]);
			if (Option == nullptr) {
				continue;
			}

			const bool IsSelected = (std::to_underlying(Selected) == Idx);
			if (ImGui::Selectable(Option, IsSelected)) {
				if (std::to_underlying(Selected) != Idx) {
					Updated = true;
					Selected = static_cast<TEnum>(Idx);
				}
			}
		}

		ImGui::EndCombo();
		return Updated;
	}

	namespace Widget::Combo {
		void TextureModifier();
		bool TextureDropdown(ETexture& Selected);
		bool BlendFunction(float IndentX = 0.0f);
		bool DepthFunction(float IndentX = 0.0f);
	}

}
