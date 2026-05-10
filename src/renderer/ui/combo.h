#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/core.h"
#include "renderer/texture.h"

namespace platformer2d::UI {

	template<std::size_t N, typename TEnum>
	inline bool Combo(std::string_view Label, const std::array<TEnum, N>& Options, TEnum& Selected)
	{
		static const auto EnumNames = Enum::View<TEnum, const char*>();
		const auto SelectedIdx = std::to_underlying(Selected);

		char NameBuf[64] = {0};
		std::snprintf(NameBuf, sizeof(NameBuf), "%s", Label.data());
		if (!ImGui::BeginCombo(NameBuf, EnumNames[SelectedIdx])) {
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

	template<typename TEnum>
	inline bool Combo(std::string_view Label, const char*& Options, const std::size_t N, TEnum& Selected)
	{
		static const auto EnumNames = Enum::View<TEnum, const char*>();
		const auto SelectedIdx = std::to_underlying(Selected);

		char NameBuf[64] = {0};
		std::snprintf(NameBuf, sizeof(NameBuf), "%s", Label.data());
		if (!ImGui::BeginCombo(NameBuf, EnumNames[SelectedIdx])) {
			return false;
		}

		bool Updated = false;
		for (std::size_t Idx = 0; Idx < N; Idx++) {
			const char* Option = Options[Idx];
			if (Option == nullptr) {
				continue;
			}

			const bool IsSelected = (SelectedIdx == Idx);
			if (ImGui::Selectable(Option, IsSelected)) {
				if (SelectedIdx != Idx) {
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
