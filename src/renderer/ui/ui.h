#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/core.h"
#include "core/enum.h"
#include "core/delegate.h"
#include "core/math/math.h"
#include "game/enemy.h"
#include "renderer/color.h"
#include "renderer/font.h"
#include "renderer/texture.h"
#include "renderer/ui/ui_core.h"
#include "renderer/ui/scoped.h"
#include "scene/actor.h"
#include "combo.h"
#include "creatorpanel.h"
#include "levellauncher.h"
#include "pausemenu.h"
#include "physics.h"
#include "text.h"

namespace platformer2d {
	class CPlayer;
	class CScene;
	class CRifle;
}

namespace platformer2d::UI {

	/* @todo: Use global config */
	inline constexpr float GAME_MENU_LABEL_COLUMN_WIDTH = 190.0f;
	inline constexpr float GAME_MENU_LABEL_INDENT_WIDTH = 24.0f;
	inline constexpr float GAME_MENU_COLUMN_ITEM_WIDTH = 410.0f;

	enum class EWidgetPlacement
	{
		Center,
		TopLeft,
		TopRight,
		BottomLeft,
		BottomRight,
	};

	inline bool InTable()
	{
		return ImGui::GetCurrentTable() != nullptr;
	}

	namespace Table {
		void Label(std::string_view Str, float IndentX = 0.0f);
		void Label(std::string_view Str, EFont Font, EFontSize FontSize = EFontSize::Regular, EFontModifier FontMod = EFontModifier::Normal, float IndentX = 0.0f);
		void NextColumn();
		void NextRow();
	}

	bool Checkbox(std::string_view Str, bool& Value, const float IndentX = 6.0f);
	void DisabledCheckbox(std::string_view Str, bool& Value, const float IndentX = 6.0f);
	bool BeginPropertyGrid(float LabelColumnWidth = 180.0f, ImGuiTableColumnFlags ColumnFlags = ImGuiTableColumnFlags_None);
	void EndPropertyGrid();

	bool ColorDropdown(EColor& Selected, float ComboWidth = -1.0f);

	bool DrawGizmo(std::uint32_t Operation, CActor& Actor, const glm::mat4& ViewMatrix,
		const glm::mat4& ProjectionMatrix, const glm::vec3& CameraPos = glm::vec3(0.0f, 0.0f, 0.0f));
	bool DrawTranslateGizmo(glm::vec2& Position, const glm::mat4& ViewMatrix, const glm::mat4& ProjectionMatrix);
	void DrawDivider(float Width, std::uint32_t Color);

	void PlayerData(std::shared_ptr<CPlayer>& Player);
	void RifleData(std::shared_ptr<CRifle> Rifle);
	void Statistics(EWidgetPlacement Placement = EWidgetPlacement::TopLeft);
	void InputDebug();
	void EnemiesInfo(std::shared_ptr<CScene>& Scene);
	void SpriteSheetModification(std::shared_ptr<CPlayer>& Player);

	namespace Input {
		bool ActorName(CActor& Actor);
	}

	bool PickImageFile(std::filesystem::path& OutPath);

	void PrepareLeftSidebar();
	void PrepareRightSidebar();
	void PrepareTopBar();
	void PrepareMenuBar();
	void PrepareBottomBar();
}

