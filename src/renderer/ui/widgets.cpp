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
		FTransformComponent& TC = Actor.GetTransformComponent();

		/* Transform Component */
		ImGui::TableNextRow();
		glm::vec3 Translation = TC.GetTranslation();
		Changed |= UI::Draw::Vec2Control("Translation", Translation, 0.010f, 0.010f);
		if (Changed)
		{
			Actor.SetPosition({ Translation.x, Translation.y });
		}

		/* Rotation */
		ImGui::TableNextRow();
		float Rotation = glm::degrees(TC.GetRotation2D());
		Changed |= UI::Draw::DragFloat("Rotation", &Rotation, 0.10f, (-6 * 360.0f), (6 * 360.0f), "%.2f");
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

		/* Tick info. */
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		UI::ShiftCursor(17.0f, 4.0f);
		ImGui::Text("Tick");

		ImGui::TableSetColumnIndex(1);
		UI::ShiftCursor(0.0f, 4.0f);
		ImGui::Text("%s", Actor.IsTickEnabled() ? "Enabled" : "Disabled");

		ImGui::EndTable();

		/* Delete actor. */
		UI::ShiftCursor(0.0f, 4.0f);
		{
			UI::FScopedFont Font(UI::Font::Get(EFont::SourceSansPro, EFontSize::Regular, EFontModifier::Bold));
			UI::FScopedStyle ButtonFrame(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
			UI::FScopedStyle ButtonRounding(ImGuiStyleVar_FrameRounding, 8);
			UI::FScopedColorStack ButtonColours(
				ImGuiCol_ButtonHovered, RGBA32::DarkRed,
				ImGuiCol_ButtonActive, RGBA32::Red
			);

			static constexpr ImVec2 ButtonSize = ImVec2(82, 42);
			const ImVec2 Avail = ImGui::GetContentRegionAvail();
			UI::ShiftCursorX(Avail.x - ButtonSize.x);
			const bool IsDeletable = Actor.IsDeletable();
			if (!IsDeletable)
			{
				ImGui::BeginDisabled();
			}
			if (ImGui::Button("Delete", ButtonSize))
			{
				CActor::OnActorMarkedForDeletion.Broadcast(Actor.GetHandle());
			}
			if (!IsDeletable)
			{
				ImGui::EndDisabled();
			}
		}
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
