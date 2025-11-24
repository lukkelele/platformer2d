#include "selectionpanel.h"

#include "core/window.h"
#include "core/selectioncontext.h"
#include "core/input/keyboard.h"
#include "game/gameinstance.h"
#include "renderer/color.h"
#include "renderer/font.h"
#include "renderer/renderer.h"
#include "ui_core.h"
#include "widgets.h"
#include "scene/scene.h"

namespace platformer2d::UI {

	void SelectionPanel()
	{
		CGameInstance* GameInstance = CGameInstance::Get();
		if (!GameInstance)
		{
			return;
		}

		std::shared_ptr<CScene> Scene = GameInstance->GetScene();
		if (!Scene)
		{
			return;
		}

		if (!ImGui::Begin("Selection", nullptr))
		{
			ImGui::End();
			return;
		}

		const LUUID SelectedID = CSelectionContext::GetSelected();

#if 0
		static constexpr float LabelColumnWidth = 180.0f;
		ImGui::BeginTable("##VectorControl", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip);
		ImGui::TableSetupColumn("Label", 0, LabelColumnWidth);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, ImGui::GetContentRegionAvail().x - LabelColumnWidth);

		auto Label = [](std::string_view Str) -> void
		{
			ImGui::TableSetColumnIndex(0);
			UI::ShiftCursor(17.0f, 4.0f);
			ImGui::Text(Str.data());
		};

		auto NextColumn = []() -> void
		{
			ImGui::TableSetColumnIndex(1);
			UI::ShiftCursor(0.0f, 4.0f);
		};

		ImGui::TableNextRow();
		if (std::shared_ptr<CActor> Actor = Scene->FindActor(SelectedID); Actor != nullptr)
		{
			Label("Name");
			NextColumn();
			ImGui::Text("%s", Actor->GetName().data());
		}
		else
		{
			Label("Name");
			NextColumn();
			ImGui::Text("None");
		}

		ImGui::EndTable();
#endif
		if (std::shared_ptr<CActor> Actor = Scene->FindActor(SelectedID); Actor != nullptr)
		{
			UI::Draw::ActorNode_Data(*Actor);
			UI::Draw::ActorNode_Buttons(*Actor, Scene);
		}
		else
		{
			ImGui::Dummy(ImVec2(0, 4));
		}

		ImGui::End(); /* ~SelectionPanel */
	}

}
