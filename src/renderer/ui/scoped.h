#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/core.h"
#include "core/delegate.h"
#include "renderer/font.h"

namespace platformer2d::UI {

	class FScopedStyle
	{
	public:
		template<typename T>
		FScopedStyle(const ImGuiStyleVar StyleVar, const T Value) { ImGui::PushStyleVar(StyleVar, Value); }
		~FScopedStyle() { ImGui::PopStyleVar(); }

	private:
		FScopedStyle(const FScopedStyle&) = delete;
		FScopedStyle& operator=(const FScopedStyle&) = delete;
	};

	class FScopedColor
	{
	public:
		template<typename T>
		FScopedColor(const ImGuiCol ColorID, const T Color) { ImGui::PushStyleColor(ColorID, ImColor(Color).Value); }
		~FScopedColor() { ImGui::PopStyleColor(); }
		FScopedColor(FScopedColor&&) = delete;
		FScopedColor(const FScopedColor&) = delete;
		FScopedColor& operator=(FScopedColor&&) = delete;
		FScopedColor& operator=(const FScopedColor&) = delete;
	};

	class FScopedID
	{
	public:
		template<typename T>
		FScopedID(const T ID) { ImGui::PushID(ID); }
		~FScopedID() { ImGui::PopID(); }
		FScopedID(FScopedID&) = delete;
		FScopedID(const FScopedID&) = delete;
		FScopedID& operator=(FScopedID&&) = delete;
		FScopedID& operator=(const FScopedID&) = delete;
	};

	class FScopedColorStack
	{
	public:
		template<typename ColorType, typename... OtherColors>
		FScopedColorStack(const ImGuiCol FirstColorID,
			const ColorType FirstColor,
			OtherColors&&... OtherColorPairs)
			: Count((sizeof...(OtherColorPairs) / 2) + 1)
		{
			static_assert((sizeof...(OtherColorPairs) & 1U) == 0, "FScopedColorStack expects a list of pairs of color IDs and colors");
			PushColor(FirstColorID, FirstColor, std::forward<OtherColors>(OtherColorPairs)...);
		}

		~FScopedColorStack() { ImGui::PopStyleColor(Count); }
		FScopedColorStack(FScopedColorStack&&) = delete;
		FScopedColorStack(const FScopedColorStack&) = delete;
		FScopedColorStack& operator=(FScopedColorStack&&) = delete;
		FScopedColorStack& operator=(const FScopedColorStack&) = delete;

	private:
		int Count = 0;

		template<typename ColorType, typename... OtherColors>
		void PushColor(const ImGuiCol ColorID,
			const ColorType Color,
			OtherColors&&... OtherColorPairs)
		{
			if constexpr (sizeof...(OtherColorPairs) == 0) {
				ImGui::PushStyleColor(ColorID, ImColor(Color).Value);
			} else {
				ImGui::PushStyleColor(ColorID, ImColor(Color).Value);
				PushColor(std::forward<OtherColors>(OtherColorPairs)...);
			}
		}
	};

	class FScopedStyleStack
	{
	public:
		template<typename ValueType, typename... OtherStylePairs>
		FScopedStyleStack(const ImGuiStyleVar FirstStyleVar,
			const ValueType FirstValue,
			OtherStylePairs&&... OtherPairs)
			: StackCount((sizeof...(OtherPairs) / 2) + 1)
		{
			static_assert((sizeof...(OtherPairs) & 1U) == 0);
			PushStyle(FirstStyleVar, FirstValue, std::forward<OtherStylePairs>(OtherPairs)...);
		}

		~FScopedStyleStack() { ImGui::PopStyleVar(StackCount); }
		FScopedStyleStack(FScopedStyleStack&&) = delete;
		FScopedStyleStack(const FScopedStyleStack&) = delete;
		FScopedStyleStack& operator=(FScopedStyleStack&&) = delete;
		FScopedStyleStack& operator=(const FScopedStyleStack&) = delete;

	private:
		int StackCount = 0;

		template<typename ValueType, typename... OtherStylePairs>
		void PushStyle(const ImGuiStyleVar StyleVar,
			const ValueType Value,
			OtherStylePairs&&... OtherPairs)
		{
			if constexpr (sizeof...(OtherPairs) == 0) {
				ImGui::PushStyleVar(StyleVar, Value);
			} else {
				ImGui::PushStyleVar(StyleVar, Value);
				PushStyle(std::forward<OtherStylePairs>(OtherPairs)...);
			}
		}
	};

	class FScopedFont
	{
	public:
		FScopedFont(ImFont* Font) { ImGui::PushFont(Font); }
		FScopedFont(const EFont Font, const EFontSize Size = EFontSize::Regular, const EFontModifier Modifier = EFontModifier::Normal)
		{
			ImGui::PushFont(UI::Font::Get(Font, Size, Modifier));
		}
		FScopedFont(const EFontSize Size = EFontSize::Regular, const EFontModifier Modifier = EFontModifier::Normal)
		{
			ImGui::PushFont(UI::Font::Get(UI::Font::GetDefault(), Size, Modifier));
		}
		FScopedFont(const EFontModifier Modifier = EFontModifier::Normal, const EFontSize Size = EFontSize::Regular)
		{
			ImGui::PushFont(UI::Font::Get(UI::Font::GetDefault(), Size, Modifier));
		}
		~FScopedFont() { ImGui::PopFont(); }
		FScopedFont(FScopedFont&&) = delete;
		FScopedFont(const FScopedFont&) = delete;
		FScopedFont& operator=(FScopedFont&&) = delete;
		FScopedFont& operator=(const FScopedFont&) = delete;
	};

}
