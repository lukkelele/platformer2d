#pragma once

#include <cstdint>

#include <imgui/imgui.h>

#include "core/enum.h"

namespace platformer2d::UI {

	enum class ESettingsCategory : std::uint8_t
	{
		Window,
		Graphics,
		Input,
		Gameplay,
		Renderer,
		Camera,
		Debug,
		COUNT,
	};
	LK_ENUM(ESettingsCategory);

	struct FSettingsMenuState
	{
		ESettingsCategory Category = ESettingsCategory::Window;
		bool bStyleEditor = false;
		bool bIDStackTool = false;
		bool bDebugBorders = false;
		bool bContentScrollable = false;

		float SaveFlashTime = 0.0f;
		bool bLastSaveOk = true;
	};

	void DrawSettingsPanel(FSettingsMenuState& State);
	void DrawSettingsDebugTools(FSettingsMenuState& State);
	bool DrawSaveButton(FSettingsMenuState& State, const ImVec2& Size);
}
