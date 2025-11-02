#pragma once

#include <array>

#include "ui_core.h"
#include "scoped.h"
#include "scene/actor.h"

namespace platformer2d::UI {

	enum class EVectorSemantic
	{
		RGB,
		XYZ,
	};

	namespace Draw
	{
		void ActorNode_VectorControl(CActor& Actor);
		void ActorNode(CActor& Actor);

		namespace _Internal
		{
		}

		inline bool DragFloat(const char* Label, float* Value, float ValueSpeed = 1.0f, float ValueMin = 0.0f, float ValueMax = 0.0f,
							  const char* Format = "%.3f", ImGuiSliderFlags Flags = 0)
		{
			static constexpr int LABEL_BUFSIZE = 72;

			const int LabelSize = std::strlen(Label);
			std::array<char, LABEL_BUFSIZE> LabelBuf{};
			std::snprintf(LabelBuf.data(), LabelBuf.size(), "##%s", Label);

			if (ImGui::GetCurrentTable() != nullptr)
			{
				if ((LabelSize > 0) && (Label[0] != '#'))
				{
					ImGui::TableSetColumnIndex(0);
					UI::ShiftCursor(17.0f, 4.0f);
					ImGui::Text(Label);
				}

				ImGui::TableSetColumnIndex(1);
				UI::ShiftCursor(0.0f, 4.0f);
			}
			else
			{
				if ((LabelSize > 0) && (Label[0] != '#'))
				{
					ImGui::Text(Label);
					ImGui::SameLine();
				}
			}

			const bool Dragged = ImGui::DragScalar(
				LabelBuf.data(), 
				ImGuiDataType_Float, 
				Value, 
				ValueSpeed, 
				&ValueMin, 
				&ValueMax, 
				Format, 
				Flags
			);

			return Dragged;
		}

		template<EVectorSemantic VecSemantic = EVectorSemantic::XYZ, typename VectorType = glm::vec2>
		inline bool Vec2Control(const std::string& Label, VectorType& Values, const float ResetValue = 0.0f,
								const float ValueSpeed = 0.10f, const float ValueMin = 0.0f, const float ValueMax = 0.0f,
								const float ColumnWidth = 100.0f, const char* Format = "%.2f")
		{
			static constexpr const char* V1 = (VecSemantic == EVectorSemantic::XYZ) ? "X" : "R";
			static constexpr const char* V2 = (VecSemantic == EVectorSemantic::XYZ) ? "Y" : "G";

			bool Modified = false;

			/* Column 0 */
			ImGui::TableSetColumnIndex(0);
			UI::ShiftCursor(17.0f, 7.0f);
			ImGui::Text(Label.c_str());

			/* Column 1 */
			ImGui::TableSetColumnIndex(1);
			UI::ShiftCursor(7.0f, 0.0f);
			{
				static constexpr float SpacingX = 8.0f;
				UI::FScopedStyle ItemSpacing(ImGuiStyleVar_ItemSpacing, ImVec2(SpacingX, 0.0f));
				UI::FScopedStyle Padding(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 2.0f));
				{
					UI::FScopedColor Padding(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
					UI::FScopedColor Frame(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));
					ImGui::BeginChild(
						ImGui::GetID((Label + "Subwindow").c_str()),
						ImVec2((ImGui::GetContentRegionAvail().x - SpacingX), ImGui::GetFrameHeightWithSpacing() + 8.0f),
						ImGuiChildFlags_None,
						ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse /* Window Flags. */
					);
				}

				static constexpr float FramePadding = 4.0f;
				static constexpr float OutlineSpacing = 1.0f;
				const float LineHeight = GImGui->Font->LegacySize + FramePadding * 2.0f;
				const ImVec2 ButtonSize = { LineHeight + 2.0f, LineHeight - 2.0f };
				const float InputItemWidth = (ImGui::GetContentRegionAvail().x - SpacingX) / 3.0f - ButtonSize.x;

				UI::ShiftCursor(0.0f, FramePadding);

				auto DrawControl = [&](const std::string& InLabel,
									   float& InValue,
									   const ImVec4& InColorNormal,
									   const ImVec4& InColorHover,
									   const ImVec4& InColorPressed)
				{
					{
						UI::FScopedStyle ButtonFrame(ImGuiStyleVar_FramePadding, ImVec2(FramePadding, 0.0f));
						UI::FScopedStyle ButtonRounding(ImGuiStyleVar_FrameRounding, 1.0f);
						UI::FScopedColorStack ButtonColours(
							ImGuiCol_Button, InColorNormal,
							ImGuiCol_ButtonHovered, InColorHover,
							ImGuiCol_ButtonActive, InColorPressed
						);

						if (ImGui::Button(InLabel.c_str(), ButtonSize))
						{
							InValue = ResetValue;
							Modified = true;
						}
					}

					ImGui::SameLine(0.0f, OutlineSpacing);
					ImGui::SetNextItemWidth(InputItemWidth);

					const ImGuiID InputID = ImGui::GetID(("##" + InLabel).c_str());
					const bool WasTempInputActive = ImGui::TempInputIsActive(InputID);
					Modified |= ImGui::DragFloat(("##" + InLabel).c_str(), &InValue, ValueSpeed, ValueMin, ValueMax, Format, 0);

					if (ImGui::TempInputIsActive(InputID))
					{
						Modified = false;
					}
				};

				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);

				/* Draw: V1 (first vector). */
				static constexpr float Diff = 0.08f;
				const ImVec4 ColorX = FColor::Convert<ImVec4>(RGBA32::DarkCyan);
				DrawControl(
					V1,
					Values.x,
					ColorX,
					{ ColorX.x + Diff, ColorX.y + Diff, ColorX.z + Diff, ColorX.w },
					{ ColorX.x - Diff, ColorX.y - Diff, ColorX.z - Diff, ColorX.w }
				);

				/* Draw: V2 (second vector). */
				ImGui::SameLine(0.0f, OutlineSpacing);
				const ImVec4 ColorY = FColor::Convert<ImVec4>(RGBA32::BrightGreen);
				DrawControl(
					V2,
					Values.y,
					ColorY,
					{ ColorY.x + Diff, ColorY.y + Diff, ColorY.z + Diff, ColorY.w },
					{ ColorY.x - Diff, ColorY.y - Diff, ColorY.z - Diff, ColorY.w }
				);

				ImGui::PopStyleVar(1); /* FrameRounding */

				ImGui::EndChild();
			}

			return Modified;
		}


		template<EVectorSemantic VecSemantic = EVectorSemantic::XYZ, typename VectorType = glm::vec3>
		inline bool Vec3Control(const std::string& Label, VectorType& Values, const float ResetValue = 0.0f,
								const float ValueSpeed = 0.10f, const float ValueMin = 0.0f, 
								const float ValueMax = 0.0f, const float ColumnWidth = 100.0f, 
								const char* Format = "%.2f")
		{
			static constexpr const char* V1 = (VecSemantic == EVectorSemantic::XYZ) ? "X" : "R";
			static constexpr const char* V2 = (VecSemantic == EVectorSemantic::XYZ) ? "Y" : "G";
			static constexpr const char* V3 = (VecSemantic == EVectorSemantic::XYZ) ? "Z" : "B";

			bool Modified = false;

			/* Column 0 */
			ImGui::TableSetColumnIndex(0);
			UI::ShiftCursor(17.0f, 7.0f);
			ImGui::Text(Label.c_str());

			/* Column 1 */
			ImGui::TableSetColumnIndex(1);
			UI::ShiftCursor(7.0f, 0.0f);
			{
				static constexpr float SpacingX = 8.0f;
				UI::FScopedStyle ItemSpacing(ImGuiStyleVar_ItemSpacing, ImVec2(SpacingX, 0.0f));
				UI::FScopedStyle Padding(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 2.0f));
				{
					UI::FScopedColor Padding(ImGuiCol_Border, IM_COL32(0, 0, 0, 0));
					UI::FScopedColor Frame(ImGuiCol_FrameBg, IM_COL32(0, 0, 0, 0));

					ImGui::BeginChild(
						ImGui::GetID((Label + "Subwindow").c_str()),
						ImVec2((ImGui::GetContentRegionAvail().x - SpacingX), ImGui::GetFrameHeightWithSpacing() + 8.0f),
						ImGuiChildFlags_None,
						ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse /* Window Flags. */
					);
				}

				static constexpr float FramePadding = 4.0f;
				static constexpr float OutlineSpacing = 1.0f;
				const float LineHeight = GImGui->Font->LegacySize + FramePadding * 2.0f;
				const ImVec2 ButtonSize = { LineHeight + 2.0f, LineHeight - 2.0f };
				const float InputItemWidth = (ImGui::GetContentRegionAvail().x - SpacingX) / 3.0f - ButtonSize.x;

				UI::ShiftCursor(0.0f, FramePadding);

				auto DrawControl = [&](const std::string& InLabel,
									   float& InValue,
									   const ImVec4& InColorNormal,
									   const ImVec4& InColorHover,
									   const ImVec4& InColorPressed)
				{
					{
						UI::FScopedStyle ButtonFrame(ImGuiStyleVar_FramePadding, ImVec2(FramePadding, 0.0f));
						UI::FScopedStyle ButtonRounding(ImGuiStyleVar_FrameRounding, 1.0f);
						UI::FScopedColorStack ButtonColours(
							ImGuiCol_Button, InColorNormal,
							ImGuiCol_ButtonHovered, InColorHover,
							ImGuiCol_ButtonActive, InColorPressed
						);

						if (ImGui::Button(InLabel.c_str(), ButtonSize))
						{
							InValue = ResetValue;
							Modified = true;
						}
					}

					ImGui::SameLine(0.0f, OutlineSpacing);
					ImGui::SetNextItemWidth(InputItemWidth);

					const ImGuiID InputID = ImGui::GetID(("##" + InLabel).c_str());
					const bool WasTempInputActive = ImGui::TempInputIsActive(InputID);
					Modified |= ImGui::DragFloat(("##" + InLabel).c_str(), &InValue, ValueSpeed, ValueMin, ValueMax, Format, 0);

					if (ImGui::TempInputIsActive(InputID))
					{
						Modified = false;
					}
				};

				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);

				/* Draw: V1 (first vector). */
				DrawControl(
					V1,
					Values.x,
					ImVec4(0.80f, 0.10f, 0.15f, 1.0f), /* Normal  */
					ImVec4(0.90f, 0.20f, 0.20f, 1.0f), /* Hover   */
					ImVec4(0.80f, 0.10f, 0.15f, 1.0f)  /* Pressed */
				);

				/* Draw: V2 (second vector). */
				ImGui::SameLine(0.0f, OutlineSpacing);
				DrawControl(
					V2,
					Values.y,
					ImVec4(0.20f, 0.70f, 0.20f, 1.0f),
					ImVec4(0.30f, 0.80f, 0.30f, 1.0f),
					ImVec4(0.20f, 0.70f, 0.20f, 1.0f)
				);

				/* Draw: V3 (third vector). */
				ImGui::SameLine(0.0f, OutlineSpacing);
				DrawControl(
					V3,
					Values.z,
					ImVec4(0.10f, 0.25f, 0.80f, 1.0f),
					ImVec4(0.20f, 0.35f, 0.90f, 1.0f),
					ImVec4(0.10f, 0.25f, 0.80f, 1.0f)
				);

				ImGui::PopStyleVar(1); /* FrameRounding */
				ImGui::EndChild();
			}

			return Modified;
		}
	}

}
