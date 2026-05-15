#pragma once

#include <array>

#include "ui_core.h"
#include "ui.h"
#include "scoped.h"
#include "core/selectioncontext.h"
#include "scene/actor.h"

namespace platformer2d {
	class CScene;
}

namespace platformer2d::UI {

	enum class EVectorSemantic
	{
		RGB,
		XYZ,
	};

	enum class EPlacementPolicy
	{
		Auto,
		InPlace, /* Do not adjust based on current widget environment (e.g if in a table) */
	};

	namespace Actor {
		void DeleteButton(std::shared_ptr<CActor> Actor, std::shared_ptr<CScene> Scene);
		void Buttons(std::shared_ptr<CActor> Actor, std::shared_ptr<CScene> Scene);
		void Data(std::shared_ptr<CActor> Actor); /* @fixme: RENAME */
		void Entry(std::shared_ptr<CActor> Actor, std::shared_ptr<CScene> Scene);

		/**
		 * @brief Update the actor cache.
		 */
		void OnActorDeleted(LUUID ActorHandle);
	}

	template<EPlacementPolicy Policy = EPlacementPolicy::Auto>
	inline bool DragFloat(const char* Label, float& Value, float ValueSpeed = 1.0f,
		float ValueMin = 0.0f, float ValueMax = 0.0f,
		const char* Format = "%.3f", ImGuiSliderFlags Flags = 0)
	{
		constexpr int LABEL_BUFSIZE = 72;

		const int LabelSize = std::strlen(Label);
		std::array<char, LABEL_BUFSIZE> LabelBuf{};
		std::snprintf(LabelBuf.data(), LabelBuf.size(), "##%s", Label);

		constexpr float SpacingX = 8.0f;
		constexpr float FramePadding = 4.0f;
		constexpr float OutlineSpacing = 1.0f;
		const float LineHeight = GImGui->Font->LegacySize + FramePadding * 2.0f;
		const ImVec2 ButtonSize = {LineHeight + 2.0f, LineHeight - 2.0f};

		UI::FScopedStyle ItemSpacing(ImGuiStyleVar_ItemSpacing, ImVec2(SpacingX, 0.0f));
		if constexpr (Policy == EPlacementPolicy::Auto) {
			if (ImGui::GetCurrentTable() != nullptr) {
				if ((LabelSize > 0) && (Label[0] != '#')) {
					ImGui::TableSetColumnIndex(0);
					UI::ShiftCursorX(17);
					ImGui::Text(Label);
				}

				ImGui::TableSetColumnIndex(1);
				UI::ShiftCursorX(7);
			} else {
				if ((LabelSize > 0) && (Label[0] != '#')) {
					ImGui::Text(Label);
					ImGui::SameLine();
				}
			}
		} else if constexpr (Policy == EPlacementPolicy::InPlace) {
			ImGui::Text(Label);
			ImGui::SameLine();
		}

		const float InputItemWidth = ((ImGui::GetContentRegionAvail().x - SpacingX) / 2.0f);
		ImGui::SetNextItemWidth(InputItemWidth);
		const ImGuiID InputID = ImGui::GetID(LabelBuf.data());
		bool Modified = ImGui::DragScalar(
			LabelBuf.data(),
			ImGuiDataType_Float,
			&Value,
			ValueSpeed,
			&ValueMin,
			&ValueMax,
			Format,
			Flags);

		if (ImGui::TempInputIsActive(InputID)) {
			Modified = false;
		}

		return Modified;
	}

	template<EVectorSemantic VecSemantic = EVectorSemantic::XYZ, typename VectorType = glm::vec2>
	inline bool DragFloat2(const std::string& Label, VectorType& Values, const float ResetValue = 0.0f,
		const float ValueSpeed = 0.10f, const float ValueMin = 0.0f, const float ValueMax = 0.0f,
		const float ColumnWidth = 100.0f, const char* Format = "%.2f")
	{
		constexpr const char* V1 = (VecSemantic == EVectorSemantic::XYZ) ? "X" : "R";
		constexpr const char* V2 = (VecSemantic == EVectorSemantic::XYZ) ? "Y" : "G";

		bool Modified = false;

		if (ImGui::GetCurrentTable() != nullptr) {
			ImGui::TableSetColumnIndex(0);
			UI::ShiftCursorX(17);
			ImGui::Text(Label.c_str());

			ImGui::TableSetColumnIndex(1);
			UI::ShiftCursorX(7);
		} else {
			ImGui::Text(Label.c_str());
			ImGui::SameLine();
		}

		UI::FScopedStyle FramePad(ImGuiStyleVar_FramePadding, ImVec2(8, 8));
		{
			static constexpr float SpacingX = 8.0f;
			UI::FScopedStyle ItemSpacing(ImGuiStyleVar_ItemSpacing, ImVec2(SpacingX, 0.0f));
			UI::FScopedStyle WindowPadding(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			{
				UI::FScopedColor BorderPadColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
				UI::FScopedColor FrameBg(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
				ImGui::BeginChild(
					ImGui::GetID((Label + "Subwindow").c_str()),
					ImVec2(ImGui::GetContentRegionAvail().x - SpacingX, ImGui::GetFrameHeightWithSpacing()),
					ImGuiChildFlags_None,
					ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse /* Window Flags. */
				);
			}

			constexpr float FramePadding = 4.0f;
			constexpr float OutlineSpacing = 1.0f;
			const float LineHeight = GImGui->Font->LegacySize + (FramePadding * 2.0f);
			const ImVec2 ButtonSize = {LineHeight + 2.0f, LineHeight - 2.0f};
			const float InputItemWidth = ((ImGui::GetContentRegionAvail().x - SpacingX) / 2.0f) - ButtonSize.x;

			auto DrawControl = [&](const std::string& InLabel,
								   float& InValue,
								   const ImVec4& InColorNormal,
								   const ImVec4& InColorHover,
								   const ImVec4& InColorPressed)
			{
				{
					UI::FScopedStyle ButtonFrame(ImGuiStyleVar_FramePadding, ImVec2(FramePadding, 0.0f));
					UI::FScopedStyle ButtonRounding(ImGuiStyleVar_FrameRounding, 1.0f);
					UI::FScopedColorStack ButtonColors(
						ImGuiCol_Button, InColorNormal,
						ImGuiCol_ButtonHovered, InColorHover,
						ImGuiCol_ButtonActive, InColorPressed);

					if (ImGui::Button(InLabel.c_str(), ButtonSize)) {
						InValue = ResetValue;
						Modified = true;
					}
				}

				ImGui::SameLine(0.0f, OutlineSpacing);
				ImGui::SetNextItemWidth(InputItemWidth);

				/* @fixme: Annoying pixel difference */
				UI::FScopedStyle FramePad(ImGuiStyleVar_FramePadding, ImVec2(FramePadding, FramePadding - 1));
				const ImGuiID InputID = ImGui::GetID(("##" + InLabel).c_str());
				const bool WasTempInputActive = ImGui::TempInputIsActive(InputID);
				Modified |= ImGui::DragFloat(("##" + InLabel).c_str(), &InValue, ValueSpeed, ValueMin, ValueMax, Format, 0);

				if (ImGui::TempInputIsActive(InputID)) {
					Modified = false;
				}
			};

			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);

			/* Draw: V1 */
			constexpr float Diff = 0.08f;
			static const ImVec4 ColorX = FColor::Convert<ImVec4>(RGBA32::DarkCyan);
			DrawControl(
				V1,
				Values.x,
				ColorX,
				{ColorX.x + Diff, ColorX.y + Diff, ColorX.z + Diff, ColorX.w},
				{ColorX.x - Diff, ColorX.y - Diff, ColorX.z - Diff, ColorX.w});

			/* Draw: V2 */
			ImGui::SameLine(0.0f, OutlineSpacing);
			static const ImVec4 ColorY = FColor::Convert<ImVec4>(RGBA32::BrightGreen);
			DrawControl(
				V2,
				Values.y,
				ColorY,
				{ColorY.x + Diff, ColorY.y + Diff, ColorY.z + Diff, ColorY.w},
				{ColorY.x - Diff, ColorY.y - Diff, ColorY.z - Diff, ColorY.w});

			ImGui::PopStyleVar(1); /* FrameRounding */

			ImGui::EndChild();
		}

		return Modified;
	}

	template<EVectorSemantic VecSemantic = EVectorSemantic::XYZ, typename VectorType = glm::vec3>
	inline bool DragFloat3(const std::string& Label, VectorType& Values, const float ResetValue = 0.0f,
		const float ValueSpeed = 0.10f, const float ValueMin = 0.0f,
		const float ValueMax = 0.0f, const float ColumnWidth = 100.0f,
		const char* Format = "%.2f")
	{
		constexpr const char* V1 = (VecSemantic == EVectorSemantic::XYZ) ? "X" : "R";
		constexpr const char* V2 = (VecSemantic == EVectorSemantic::XYZ) ? "Y" : "G";
		constexpr const char* V3 = (VecSemantic == EVectorSemantic::XYZ) ? "Z" : "B";

		bool Modified = false;

		if (ImGui::GetCurrentTable() != nullptr) {
			ImGui::TableSetColumnIndex(0);
			UI::ShiftCursorX(17);
			ImGui::Text(Label.c_str());

			ImGui::TableSetColumnIndex(1);
			UI::ShiftCursorX(7);
		} else {
			ImGui::Text(Label.c_str());
			ImGui::SameLine();
		}

		UI::FScopedStyle FramePad(ImGuiStyleVar_FramePadding, ImVec2(8, 8));
		{
			static constexpr float SpacingX = 8.0f;
			UI::FScopedStyle ItemSpacing(ImGuiStyleVar_ItemSpacing, ImVec2(SpacingX, 0.0f));
			UI::FScopedStyle WindowPadding(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			{
				UI::FScopedColor BorderPadColor(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
				UI::FScopedColor FrameBg(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));

				ImGui::BeginChild(
					ImGui::GetID((Label + "Subwindow").c_str()),
					ImVec2((ImGui::GetContentRegionAvail().x - SpacingX), ImGui::GetFrameHeightWithSpacing()),
					ImGuiChildFlags_None,
					ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse /* Window Flags. */
				);
			}

			constexpr float FramePadding = 4.0f;
			constexpr float OutlineSpacing = 1.0f;
			const float LineHeight = GImGui->Font->LegacySize + (FramePadding * 2.0f);
			const ImVec2 ButtonSize = {LineHeight + 2.0f, LineHeight - 2.0f};
			const float InputItemWidth = ((ImGui::GetContentRegionAvail().x - SpacingX) / 3.0f) - ButtonSize.x;

			auto DrawControl = [&](const std::string& InLabel,
								   float& InValue,
								   const ImVec4& InColorNormal,
								   const ImVec4& InColorHover,
								   const ImVec4& InColorPressed)
			{
				{
					UI::FScopedStyle ButtonFrame(ImGuiStyleVar_FramePadding, ImVec2(FramePadding, 0.0f));
					UI::FScopedStyle ButtonRounding(ImGuiStyleVar_FrameRounding, 1.0f);
					UI::FScopedColorStack ButtonColors(
						ImGuiCol_Button, InColorNormal,
						ImGuiCol_ButtonHovered, InColorHover,
						ImGuiCol_ButtonActive, InColorPressed);

					if (ImGui::Button(InLabel.c_str(), ButtonSize)) {
						InValue = ResetValue;
						Modified = true;
					}
				}

				ImGui::SameLine(0.0f, OutlineSpacing);
				ImGui::SetNextItemWidth(InputItemWidth);

				/* @fixme: Annoying pixel difference */
				UI::FScopedStyle FramePad(ImGuiStyleVar_FramePadding, ImVec2(FramePadding, FramePadding - 1));
				const ImGuiID InputID = ImGui::GetID(("##" + InLabel).c_str());
				const bool WasTempInputActive = ImGui::TempInputIsActive(InputID);
				Modified |= ImGui::DragFloat(("##" + InLabel).c_str(), &InValue, ValueSpeed, ValueMin, ValueMax, Format, 0);

				if (ImGui::TempInputIsActive(InputID)) {
					Modified = false;
				}
			};

			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);

			/* Draw: V1 */
			DrawControl(
				V1,
				Values.x,
				ImVec4(0.80f, 0.10f, 0.15f, 1.0f), /* Normal  */
				ImVec4(0.90f, 0.20f, 0.20f, 1.0f), /* Hover   */
				ImVec4(0.80f, 0.10f, 0.15f, 1.0f) /* Pressed */
			);

			/* Draw: V2 */
			ImGui::SameLine(0.0f, OutlineSpacing);
			DrawControl(
				V2,
				Values.y,
				ImVec4(0.20f, 0.70f, 0.20f, 1.0f),
				ImVec4(0.30f, 0.80f, 0.30f, 1.0f),
				ImVec4(0.20f, 0.70f, 0.20f, 1.0f));

			/* Draw: V3 */
			ImGui::SameLine(0.0f, OutlineSpacing);
			DrawControl(
				V3,
				Values.z,
				ImVec4(0.10f, 0.25f, 0.80f, 1.0f),
				ImVec4(0.20f, 0.35f, 0.90f, 1.0f),
				ImVec4(0.10f, 0.25f, 0.80f, 1.0f));

			ImGui::PopStyleVar(1); /* FrameRounding */
			ImGui::EndChild();
		}

		return Modified;
	}

	template<typename TComponent, typename TUIFunction>
	inline void DrawComponent(const std::string& ComponentName, std::shared_ptr<CActor> Actor, TUIFunction UIFunction)
	{
		if (!Actor || !Actor->HasComponent<TComponent>()) {
			return;
		}

		constexpr ImGuiTreeNodeFlags TreeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen
			| ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth
			| ImGuiTreeNodeFlags_AllowOverlap | ImGuiTreeNodeFlags_FramePadding;
		auto& Comp = Actor->GetComponent<TComponent>();

		ImGuiStyle& Style = ImGui::GetStyle();
		const bool DisplayFrame = (TreeNodeFlags & ImGuiTreeNodeFlags_Framed) != 0;
		const ImVec2 Padding = (DisplayFrame || (TreeNodeFlags & ImGuiTreeNodeFlags_FramePadding)) ? Style.FramePadding : ImVec2(Style.FramePadding.x, ImMin(ImGui::GetCurrentWindow()->DC.CurrLineTextBaseOffset, Style.FramePadding.y));
		const float LineHeight = GImGui->FontSize + GImGui->Style.FramePadding.y + Padding.y;

		const ImVec2 ContentRegionAvailable = ImGui::GetContentRegionAvail();
		const bool Open = ImGui::TreeNodeEx((void*)typeid(TComponent).hash_code(), TreeNodeFlags, ComponentName.c_str());
		ImGui::SameLine(ContentRegionAvailable.x - LineHeight * 0.50f);
		if (ImGui::Button("+", ImVec2(LineHeight, LineHeight))) {
			ImGui::OpenPopup("ComponentSettings");
		}

		bool RemoveComponent = false;
		if (ImGui::BeginPopup("ComponentSettings")) {
			if (ImGui::MenuItem("Remove Component")) {
				RemoveComponent = true;
			}
			ImGui::EndPopup();
		}

		if (Open) {
			UIFunction(Comp);
			ImGui::TreePop();
		}

		if (RemoveComponent) {
			LK_DEBUG_TAG("UI", "Removing component from: {}", Actor->GetName());
			if (!Actor->RemoveComponent<TComponent>()) {
				LK_ERROR_TAG("UI", "Failed to remove component from {}", Actor->GetName());
			}
		}
	}

	template<typename TComponent, typename... TIncompatible>
	void DrawAddComponentButton(std::string_view Name, std::shared_ptr<CTexture> Icon, std::shared_ptr<CActor> Actor)
	{
		static constexpr float RowHeight = 25.0f;
		auto* Window = ImGui::GetCurrentWindow();
		Window->DC.CurrLineSize.y = RowHeight;
		ImGui::TableNextRow(0, RowHeight);
		ImGui::TableSetColumnIndex(0);

		Window->DC.CurrLineTextBaseOffset = 3.0f;

		const ImVec2 RowAreaMin = ImGui::TableGetCellBgRect(ImGui::GetCurrentTable(), 0).Min;
		const ImVec2 RowAreaMax = {ImGui::TableGetCellBgRect(ImGui::GetCurrentTable(), ImGui::TableGetColumnCount() - 1).Max.x - 20,
			RowAreaMin.y + RowHeight};

		ImGuiID ID = ImGui::GetID(Name.data());
		ImRect RowRect(RowAreaMin, RowAreaMax);
		ImGui::ItemAdd(RowRect, ID);
		ImGui::PushClipRect(RowAreaMin, RowAreaMax, false);

		bool IsRowHovered = false;
		bool IsHeld = false;
		ImGui::SetNextItemAllowOverlap();
		bool IsRowClicked = ImGui::ButtonBehavior(RowRect, ID, &IsRowHovered, &IsHeld, ImGuiButtonFlags_AllowOverlap);
		ImGui::PopClipRect();

		auto FillRowWithColor = [](const ImColor& Color)
		{
			for (int Column = 0; Column < ImGui::TableGetColumnCount(); Column++) {
				ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, Color, Column);
			}
		};

		if (IsRowHovered) {
			FillRowWithColor(RGBA32::Background);
			ImGui::SetTooltip("Clicked: %d RowAreaMin=(%.2f, %.2f) RowAreaMax=(%.2f, %.2f)", IsRowClicked, RowAreaMin.x,
				RowAreaMin.y, RowAreaMax.x, RowAreaMax.y);
		}

		UI::Image(Icon, ImVec2(RowHeight - 3.0f, RowHeight - 3.0f));
		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-1);
		ImGui::TextUnformatted(Name.data());

		if (IsRowClicked) {
			if ((sizeof...(TIncompatible) > 0) && Actor->HasAnyComponents<TIncompatible...>()) {
				return;
			}

			if (!Actor->HasComponent<TComponent>()) {
				LK_INFO_TAG("UI", "Add component to {}", Actor->GetName());
				Actor->AddComponent<TComponent>();
			} else {
				LK_INFO_TAG("UI", "Actor {} already has that component", Actor->GetName());
			}

			ImGui::CloseCurrentPopup();
		}
	}

	void DrawComponents(std::shared_ptr<CActor> Actor);
	void DrawController(IController* Controller);
	void DrawEnemy(std::shared_ptr<CEnemy> Enemy);
	void Rifle(std::shared_ptr<CRifle> InRifle);
	void SceneManagerPanel(std::shared_ptr<CScene> InScene);
	void EditorViewportInfo(bool Focused, bool Hovered);
}
