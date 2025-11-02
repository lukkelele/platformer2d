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
		FORCEINLINE FScopedStyle(const ImGuiStyleVar StyleVar, const T Value) { ImGui::PushStyleVar(StyleVar, Value); }
		~FScopedStyle() { ImGui::PopStyleVar(); }

	private:
		FScopedStyle(const FScopedStyle&) = delete;
		FScopedStyle& operator=(const FScopedStyle&) = delete;
	};

	class FScopedColor
	{
	public:
		template<typename T>
		FORCEINLINE FScopedColor(const ImGuiCol ColorID, const T Color) { ImGui::PushStyleColor(ColorID, ImColor(Color).Value); }
		~FScopedColor() { ImGui::PopStyleColor(); }

	private:
		FScopedColor(const FScopedColor&) = delete;
		FScopedColor& operator=(const FScopedColor&) = delete;
	};

	class FScopedID
	{
	public:
		template<typename T>
		FORCEINLINE FScopedID(const T ID) { ImGui::PushID(ID); }
		~FScopedID() { ImGui::PopID(); }

	private:
		FScopedID(const FScopedID&) = delete;
		FScopedID& operator=(const FScopedID&) = delete;
	};

	class FScopedColorStack
	{
	public:
		template <typename ColorType, typename... OtherColors>
		FORCEINLINE FScopedColorStack(const ImGuiCol FirstColorID, 
									  const ColorType FirstColor, 
									  OtherColors&&... OtherColorPairs)
			: Count((sizeof... (OtherColorPairs) / 2) + 1)
		{
			static_assert((sizeof... (OtherColorPairs) & 1u) == 0, "FScopedColorStack expects a list of pairs of color IDs and colors");
			PushColor(FirstColorID, FirstColor, std::forward<OtherColors>(OtherColorPairs)...);
		}

		~FScopedColorStack() { ImGui::PopStyleColor(Count); }

	private:
		FScopedColorStack(const FScopedColorStack&) = delete;
		FScopedColorStack& operator=(const FScopedColorStack&) = delete;

	private:
		int Count = 0;

		template <typename ColorType, typename... OtherColors>
		FORCEINLINE void PushColor(const ImGuiCol ColorID, 
								   const ColorType Color, 
								   OtherColors&& ... OtherColorPairs)
		{
			if constexpr (sizeof... (OtherColorPairs) == 0)
			{
				ImGui::PushStyleColor(ColorID, ImColor(Color).Value);
			}
			else
			{
				ImGui::PushStyleColor(ColorID, ImColor(Color).Value);
				PushColor(std::forward<OtherColors>(OtherColorPairs)...);
			}
		}
	};

	class FScopedStyleStack
	{
	public:
		template<typename ValueType, typename... OtherStylePairs>
		FORCEINLINE FScopedStyleStack(const ImGuiStyleVar FirstStyleVar, 
									  const ValueType FirstValue, 
									  OtherStylePairs&& ... OtherPairs)
			: StackCount((sizeof...(OtherPairs) / 2) + 1)
		{
			static_assert((sizeof...(OtherPairs) & 1u) == 0);
			PushStyle(FirstStyleVar, FirstValue, std::forward<OtherStylePairs>(OtherPairs)...);
		}

		~FScopedStyleStack() { ImGui::PopStyleVar(StackCount); }

	private:
		FScopedStyleStack(const FScopedStyleStack&) = delete;
		FScopedStyleStack& operator=(const FScopedStyleStack&) = delete;

	private:
		int StackCount = 0;

		template<typename ValueType, typename... OtherStylePairs>
		FORCEINLINE void PushStyle(const ImGuiStyleVar StyleVar, 
								   const ValueType Value, 
								   OtherStylePairs&& ... OtherPairs)
		{
			if constexpr (sizeof...(OtherPairs) == 0)
			{
				ImGui::PushStyleVar(StyleVar, Value);
			}
			else
			{
				ImGui::PushStyleVar(StyleVar, Value);
				PushStyle(std::forward<OtherStylePairs>(OtherPairs)...);
			}
		}
	};

	class FScopedFont
	{
	public:
		FORCEINLINE FScopedFont(ImFont* Font) { ImGui::PushFont(Font); }
		~FScopedFont() { ImGui::PopFont(); }

	private:
		FScopedFont(const FScopedFont&) = delete;
		FScopedFont& operator=(const FScopedFont&) = delete;
	};

}