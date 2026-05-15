#include "terraincreator.h"

#include "core/selectioncontext.h"
#include "game/instance.h"
#include "game/spawner.h"
#include "renderer/renderer.h"
#include "scene/actor.h"
#include "scene/scene.h"

#include "ui.h"
#include "widgets.h"

namespace platformer2d::UI {

	FTerrainCreator TerrainCreator;

	static void ApplyChainToActor(std::shared_ptr<CActor> Actor, FTerrainCreator& State)
	{
		LK_ASSERT(Actor && (State.Points.size() >= 4));
		CBody* Body = Actor->GetBody();
		if (Body && (Body->TryGetShape<EShape::Chain>() != nullptr)) {
			Body->GetShape<EShape::Chain>().TextureHeight = State.TextureHeight;
			Body->SetChainPoints(State.Points, State.bLoop, State.bBlockBothSides);
		} else {
			FBodySpecification BodySpec;
			BodySpec.Type = EBodyType::Static;
			BodySpec.Position = Actor->GetPosition();
			BodySpec.Flags = EBodyFlag_PreSolveEvents;

			FChain Chain;
			Chain.Points = State.Points;
			Chain.bLoop = State.bLoop;
			Chain.bBlockBothSides = State.bBlockBothSides;
			Chain.TextureHeight = State.TextureHeight;
			BodySpec.Shape.emplace<FChain>(Chain);

			Actor->ReplaceBody(BodySpec);
		}
		Actor->SetColor(FColor::Get(State.Color));
		Actor->SetTexture(State.Texture);
	}

	static void UI_DebugInfo()
	{
		FTerrainCreator& State = TerrainCreator;
		if (ImGui::TreeNodeEx("Debug", ImGuiTreeNodeFlags_SpanAvailWidth)) {
			UI::BeginPropertyGrid();
			Table::NextRow();
			{
				Table::Label("Preview Origin");
				Table::NextColumn();
				ImGui::Text("(%.3f, %.3f)", State.PreviewOrigin.x, State.PreviewOrigin.y);
			}
			Table::NextRow();
			{
				Table::Label("Points");
				Table::NextColumn();
				ImGui::Text("%d", State.Points.size());
			}
			Table::NextRow();
			{
				Table::Label("Has Edit Target");
				Table::NextColumn();
				ImGui::Text("%s", State.bHasEditTarget ? "Yes" : "No");
			}

			EndPropertyGrid();
			ImGui::TreePop();
		}
	}

	static bool UI_ChainPoints()
	{
		FTerrainCreator& State = TerrainCreator;
		bool PointsModified = false;
		for (std::size_t Idx = 0; Idx < State.Points.size(); Idx++) {
			UI::FScopedID ScopedID(static_cast<int>(Idx));
			ImGui::Text("%2zu", Idx);
			ImGui::SameLine();

			ImGui::SetNextItemWidth(160.0f);
			if (ImGui::DragFloat2("##Point", &State.Points.at(Idx).x, 0.05f, 0.0f, 0.0f, "%.3f")) {
				PointsModified = true;
			}

			ImGui::SameLine();
			if (ImGui::SmallButton("Insert")) {
				const glm::vec2 InsertAt = ((Idx + 1) < State.Points.size())
					? (State.Points.at(Idx) + State.Points.at(Idx + 1)) * 0.50f
					: State.Points.at(Idx) + glm::vec2(0.50f, 0.0f);
				State.Points.insert(State.Points.begin() + Idx + 1, InsertAt);
				PointsModified = true;
				break;
			}

			ImGui::SameLine();
			const bool CanDelete = (State.Points.size() > 4);
			if (!CanDelete) {
				ImGui::BeginDisabled();
			}
			if (ImGui::SmallButton("X")) {
				State.Points.erase(State.Points.begin() + Idx);
				PointsModified = true;
				if (!CanDelete) {
					ImGui::EndDisabled();
				}
				break;
			}
			if (!CanDelete) {
				ImGui::EndDisabled();
			}
		}

		if (ImGui::Button("Append Point")) {
			const glm::vec2 Tail = State.Points.empty()
				? glm::vec2(0.0f, 0.0f)
				: (State.Points.back() + glm::vec2(0.50f, 0.0f));
			State.Points.push_back(Tail);
			PointsModified = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Reset")) {
			State.ResetPoints();
			PointsModified = true;
		}

		return PointsModified;
	}

	void RenderTerrainCreator(std::shared_ptr<CScene> Scene)
	{
		const bool Opened = UI::Begin(PanelID::TerrainCreator, nullptr);
		if (!Opened) {
			return;
		}
		if (!Scene) {
			UI::End();
			return;
		}

		FTerrainCreator& State = TerrainCreator;
		const LUUID SelectedHandle = CSelectionContext::GetSelected();
		std::shared_ptr<CActor> SelectedActor = Scene->GetActor(SelectedHandle);

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 3));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

		/* Check if any selected target to edit. */
		if ((SelectedHandle != LUUID::Null) && (SelectedHandle != State.EditTarget) && SelectedActor) {
			if (CBody* Body = SelectedActor->GetBody(); Body != nullptr) {
				if (const FChain* Chain = Body->TryGetShape<EShape::Chain>(); Chain != nullptr) {
					State.EditTarget = SelectedHandle;
					State.bHasEditTarget = true;
					State.Points = Chain->Points;
					State.bLoop = Chain->bLoop;
					State.bBlockBothSides = Chain->bBlockBothSides;
					State.TextureHeight = Chain->TextureHeight;
					State.Texture = SelectedActor->GetTexture();
					const glm::vec3 ActorPos = SelectedActor->GetPosition();
					State.PreviewOrigin = {ActorPos.x, ActorPos.y};
				}
			}
		}

		/* Edit target binding. */
		std::shared_ptr<CActor> EditActor = nullptr;
		if (State.bHasEditTarget) {
			EditActor = Scene->GetActor(State.EditTarget);
			if (!EditActor) {
				State.bHasEditTarget = false;
			}
		}

		if (EditActor) {
			const glm::vec3 ActorPos = EditActor->GetPosition();
			State.PreviewOrigin = {ActorPos.x, ActorPos.y};
		}

		const bool NewChain = (EditActor == nullptr);
		BeginPropertyGrid();
		{
			/* Row: Chain to edit */
			Table::NextRow();
			{
				Table::Label("Editing");
				Table::NextColumn();
				if (NewChain) {
					UI::FScopedFont Font(EFontModifier::Bold);
					ImGui::Text("<New chain>");
				} else {
					ImGui::Text("%s", std::string(EditActor->GetName()).c_str());
				}

				if (EditActor) {
					ImGui::SameLine();
					if (ImGui::SmallButton("Clear##ChainEdit")) {
						State.bHasEditTarget = false;
					}
				}
			}

			Table::NextRow();
			{
				Table::Label("Name");
				Table::NextColumn();
				ImGui::InputText("##NameBuf", State.NameBuf.data(), State.NameBuf.size());
			}

			Table::NextRow();
			UI::Checkbox("Show Preview", State.bPreviewVisible);

			Table::NextRow();
			UI::Checkbox("Loop", State.bLoop);

			Table::NextRow();
			if (UI::Checkbox("Block both sides", State.bBlockBothSides) && EditActor) {
				ApplyChainToActor(EditActor, State);
			}
			if (ImGui::IsItemHovered()) {
				UI::SetTooltip("Off: one-way platform (stand on top, jump up through). On: solid wall");
			}

			Table::NextRow();
			if (ColorDropdown(State.Color)) {
				if (SelectedActor) {
					SelectedActor->SetColor(FColor::Get(State.Color));
				}
			}

			Table::NextRow();
			{
				if (UI::TextureDropdown(State.Texture)) {
					if (EditActor) {
						ApplyChainToActor(EditActor, State);
					}
				}
			}

			Table::NextRow();
			{
				if (UI::DragFloat("Texture Height", State.TextureHeight, 0.01f, 0.0f, 10.0f, "%.3f")) {
					if (EditActor) {
						ApplyChainToActor(EditActor, State);
					}
				}
			}
		}
		EndPropertyGrid();

		UI_DebugInfo();
		ImGui::Dummy(ImVec2(0, 10));

		if (NewChain) {
			ImGui::Text("Spawn at");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(160.0f);
			ImGui::DragFloat2("##Origin", &State.PreviewOrigin.x, 0.05f, 0.0f, 0.0f, "%.2f");
			ImGui::SameLine();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 6));
			if (ImGui::Button("To Player")) {
				if (auto Player = CGameInstance::Get().GetPlayer(0); Player != nullptr) {
					const glm::vec3 PlayerPos = Player->GetPosition();
					State.PreviewOrigin = {PlayerPos.x, PlayerPos.y};
				}
			}
			ImGui::PopStyleVar();
		}

		ImGui::Text("Points (%zu)", State.Points.size());
		ImGui::SameLine();
		if (ImGui::Button("Reverse")) {
			std::reverse(State.Points.begin(), State.Points.end());
			if (EditActor) {
				ApplyChainToActor(EditActor, State);
			}
		}
		if (ImGui::IsItemHovered()) {
			UI::SetTooltip("Flip the side that collides for one-way chains");
		}

		const bool PointsModified = UI_ChainPoints();
		ImGui::Dummy(ImVec2(0, 10));

		const bool ValidPoints = (State.Points.size() >= 4);
		if (!ValidPoints) {
			ImGui::TextColored(FColor::Convert<ImVec4>(RGBA32::Red), "Need at least 4 points");
			ImGui::BeginDisabled();
		}
		if (EditActor) {
			if (ImGui::Button("Apply") || (PointsModified && ValidPoints)) {
				ApplyChainToActor(EditActor, State);
			}
		} else {
			if (ImGui::Button("Create Chain")) {
				std::shared_ptr<CActor> Actor = CSpawner::CreateChain(State.NameBuf.data(), State.Points, State.bLoop, State.bBlockBothSides, FColor::Get(State.Color));
				if (Actor) {
					Actor->SetPosition(State.PreviewOrigin);
					State.EditTarget = Actor->GetHandle();
					State.bHasEditTarget = true;
				}
			}
		}
		if (!ValidPoints) {
			ImGui::EndDisabled();
		}

		if (CSelectionContext::IsAnySelected()) {
			State.bPreviewVisible = ValidPoints;
			if (SelectedActor && (SelectedActor->GetTexture() == ETexture::White)) {
				EColor DeducedColor;
				if (FColor::DeduceEnum(DeducedColor, SelectedActor->GetColor())) {
					State.Color = DeducedColor;
				}
			}
		}

		ImGui::PopStyleVar(2);
		UI::End();
	}

	void RenderChainPreview(const std::shared_ptr<CScene>& Scene)
	{
		const FTerrainCreator& State = TerrainCreator;
		if (!State.bPreviewVisible || State.Points.size() < 2) {
			return;
		}

		const glm::vec4 BaseColor = FColor::Get(State.Color);
		const glm::vec4 PreviewColor(BaseColor.r, BaseColor.g, BaseColor.b, 0.70f);
		const glm::vec4 GhostColor(BaseColor.r, BaseColor.g, BaseColor.b, 0.25f);

		const glm::vec2 Origin = State.PreviewOrigin;
		const std::size_t N = State.Points.size();

		auto SubmitLine = [](const glm::vec2& A, const glm::vec2& B, const glm::vec4& Color)
		{
			const glm::vec3 P0 = {A.x, A.y, 0.0f};
			const glm::vec3 P1 = {B.x, B.y, 0.0f};
			CRenderer::DrawLine(P0, P1, Color, 3);
		};

		const std::size_t SegmentCount = (State.bLoop ? N : (N - 1));
		for (std::size_t Idx = 0; Idx < SegmentCount; Idx++) {
			const glm::vec2 A = Origin + State.Points.at(Idx);
			const glm::vec2 B = Origin + State.Points[(Idx + 1) % N];
			SubmitLine(A, B, PreviewColor);
		}

		if (!State.bLoop && (N >= 2)) {
			constexpr float F = 1.0f;
			const glm::vec2 GhostFront = Origin + (F * State.Points.front() - State.Points.at(1));
			const glm::vec2 GhostBack = Origin + (F * State.Points.back() - State.Points.at(N - 2));
			SubmitLine(Origin + State.Points.front(), GhostFront, GhostColor);
			SubmitLine(Origin + State.Points.back(), GhostBack, GhostColor);
		}
	}
}
