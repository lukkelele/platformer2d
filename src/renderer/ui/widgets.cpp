#include "widgets.h"

#include "core/selectioncontext.h"
#include "renderer/imgui.h"

namespace platformer2d::UI::Draw {

	void ActorNode_VectorControl(CActor& Actor)
	{
		ImGui::BeginTable("##VectorControl", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoClip);
		ImGui::TableSetupColumn("LabelColumn", 0, 100.0f);
		ImGui::TableSetupColumn("ValueColumn", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, ImGui::GetContentRegionAvail().x - 100.0f);

		bool Changed = false;

		/* Transform Component */
		ImGui::TableNextRow();
		FTransformComponent& TC = Actor.GetTransformComponent();
		Changed |= UI::Draw::Vec2Control("Transform", TC.Translation, 0.010f, 0.010f);
		if (Changed)
		{
			Actor.SetPosition({ TC.Translation.x, TC.Translation.y });
		}

		/* Rotation */
		ImGui::TableNextRow();
		float Rotation = glm::degrees(TC.GetRotation2D());
		Changed |= UI::Draw::DragFloat("Rotation", &Rotation, 0.10f, 0.0f, (6 * 360.0f), "%.2f");
		if (Changed)
		{
			Actor.SetRotation(glm::radians(Rotation));
		}

		/* Scale */
		ImGui::TableNextRow();
		Changed |= UI::Draw::Vec2Control("Scale", TC.Scale, 0.10f, 0.010f, 0.010f);
#if 0
		glm::vec2 Scale = TC.Scale;
		constexpr float ColumnWidth = 100.0f;
		Changed |= UI::Draw::Vec2Control("Scale", Scale, 0.10f, 0.010f, 0.010f, 0.0f, ColumnWidth);
		if (Changed)
		{
			Actor.GetBody().SetScale(TC.Scale);
		}
#endif

		ImGui::EndTable();
	}

	void ActorNode(CActor& Actor)
	{
		const FActorHandle Handle = Actor.GetHandle();
		ImGui::PushID(Handle);

		const bool bIsSelected = CSelectionContext::IsSelected(Handle);

		ImGuiTreeNodeFlags TreeNodeFlags = (bIsSelected ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None);
		TreeNodeFlags |= ImGuiTreeNodeFlags_SpanAvailWidth;

		const ImGuiID ActorImGuiID = ImGui::GetID((void*)(uint64_t)(uint32_t)Handle);

		std::string_view Name = Actor.GetName();
		char NodeName[84];
		std::snprintf(NodeName, sizeof(NodeName), "%s (%u)", Name.data(), Handle);

		const bool bWasNodeOpen = ImGui::TreeNodeBehaviorIsOpen(ActorImGuiID);
		const bool bNodeOpened = ImGui::TreeNodeEx((void*)ActorImGuiID, TreeNodeFlags, NodeName);
		if (bNodeOpened)
		{
			ActorNode_VectorControl(Actor);
			ImGui::TreePop();
		}

		ImGui::PopID();
	}

}
