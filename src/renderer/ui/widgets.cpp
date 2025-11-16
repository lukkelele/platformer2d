#include "widgets.h"

#include "core/selectioncontext.h"
#include "renderer/imgui.h"

namespace platformer2d::UI::Draw {

	void ActorNode_VectorControl(CActor& Actor)
	{
		static constexpr float LabelColumnWidth = 180.0f;
		ImGui::BeginTable("##VectorControl", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoClip);
		ImGui::TableSetupColumn("LabelColumn", 0, LabelColumnWidth);
		ImGui::TableSetupColumn("ValueColumn", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, ImGui::GetContentRegionAvail().x - LabelColumnWidth);

		bool Changed = false;
		FTransformComponent& TC = Actor.GetTransformComponent();
		const CBody& Body = Actor.GetBody();

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
		constexpr float LabelColumnWidth = 100.0f;
		Changed |= UI::Draw::Vec2Control("Scale", Scale, 0.10f, 0.010f, 0.010f, 0.0f, LabelColumnWidth);
		if (Changed)
		{
			Actor.GetBody().SetScale(TC.Scale);
		}
#endif

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

		/* Tick info. */
		ImGui::TableNextRow();
		{
			Label("Tick");
			NextColumn();
			ImGui::Text("%s", Actor.IsTickEnabled() ? "Enabled" : "Disabled");
		}

		/* Body Type. */
		ImGui::TableNextRow();
		{
			Label("Body Type");
			NextColumn();
			ImGui::Text("%s", Enum::ToString(Body.GetType()));
		}

		/* Body Size. */
		ImGui::TableNextRow();
		{
			Label("Body Size");
			NextColumn();
			const glm::vec2 Size = Body.GetSize();
			ImGui::Text("(%.2f, %.2f)", Size.x, Size.y);
		}

		/* Linear Velocity. */
		ImGui::TableNextRow();
		{
			Label("Linear Velocity");
			NextColumn();
			const glm::vec2 V = Body.GetLinearVelocity();
			ImGui::Text("(%.2f, %.2f)", V.x, V.y);
		}

		/* Angular Velocity. */
		ImGui::TableNextRow();
		{
			Label("Angular Velocity");
			NextColumn();
			const float V = Body.GetAngularVelocity();
			ImGui::Text("%.2f", V);
		}

		/* AABB. */
		ImGui::TableNextRow();
		{
			Label("AABB");
			NextColumn();
			const FAABB AABB = Body.GetAABB();
			ImGui::Text("Min (%.2f, %.2f)", AABB.Min.x, AABB.Min.y);
			ImGui::SameLine(0.0f, 16.0f);
			ImGui::Text("Max (%.2f, %.2f)", AABB.Max.x, AABB.Max.y);
		}

		/* Components. */
		/* @todo: Make treenode */
		ImGui::TableNextRow();
		{
			Label("EffectComponent");
			NextColumn();
			if (Actor.HasComponent<FEffectComponent>())
			{
				ImGui::Text("Yes");
			}
			else
			{
				ImGui::Text("No");
			}
		}

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
		const LUUID Handle = Actor.GetHandle();
		ImGui::PushID(Handle);

		const bool bIsSelected = CSelectionContext::IsSelected(Handle);

		ImGuiTreeNodeFlags TreeNodeFlags = (bIsSelected ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None);
		TreeNodeFlags |= ImGuiTreeNodeFlags_SpanAvailWidth;

		const ImGuiID ActorImGuiID = ImGui::GetID((void*)(uint64_t)(uint32_t)Handle);

		std::string_view Name = Actor.GetName();
		char NodeName[84];
		std::snprintf(NodeName, sizeof(NodeName), "%s (%lld)", Name.data(), static_cast<LUUID::SizeType>(Handle));

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
