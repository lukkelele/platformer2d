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

	static bool TerrainSection(const char* Label)
	{
		ImGui::PushFont(UI::Font::Get(EFont::SourceSansPro, EFontSize::Large, EFontModifier::Bold));
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		const bool Open = ImGui::TreeNodeEx(Label, ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding);
		ImGui::PopFont();
		return Open;
	}

	static void ApplyChainToActor(std::shared_ptr<CActor> Actor, FTerrainCreator& State)
	{
		LK_ASSERT(Actor && (State.Points.size() >= 4));
		CBody* Body = Actor->GetBody();
		if (Body && (Body->TryGetShape<EShape::Chain>() != nullptr)) {
			FChain& ChainRef = Body->GetShape<EShape::Chain>();
			ChainRef.TextureHeight = State.TextureHeight;
			ChainRef.TextureSide = State.TextureSide;
			ChainRef.TextureOffset = State.TextureOffset;
			ChainRef.bTextureTile = State.bTextureTile;
			ChainRef.TextureTileWidth = State.TextureTileWidth;
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
			Chain.TextureSide = State.TextureSide;
			Chain.TextureOffset = State.TextureOffset;
			Chain.bTextureTile = State.bTextureTile;
			Chain.TextureTileWidth = State.TextureTileWidth;
			BodySpec.Shape.emplace<FChain>(Chain);

			Actor->ReplaceBody(BodySpec);
		}

		if (Actor->GetColor() != FColor::Get(State.Color)) {
			Actor->SetColor(FColor::Get(State.Color));
		}
		if (Actor->GetTexture() != State.Texture) {
			Actor->SetTexture(State.Texture);
		}
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

		const float IconButton = ImGui::GetFrameHeight();

		for (std::size_t Idx = 0; Idx < State.Points.size(); Idx++) {
			UI::FScopedID ScopedID(static_cast<int>(Idx));

			ImGui::AlignTextToFramePadding();
			{
				UI::FScopedColor IndexColor(ImGuiCol_Text, RGBA32::Text::Darker);
				ImGui::Text("%2zu", Idx);
			}
			ImGui::SameLine(0.0f, 8.0f);

			const float Reserved = (IconButton * 2.0f) + 14.0f;
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - Reserved);
			if (ImGui::DragFloat2("##Point", &State.Points.at(Idx).x, 0.05f, 0.0f, 0.0f, "%.3f")) {
				PointsModified = true;
			}

			/* Insert after. */
			ImGui::SameLine(0.0f, 6.0f);
			{
				UI::FScopedColorStack Colors(
					ImGuiCol_Button, RGBA32::BackgroundDark,
					ImGuiCol_ButtonHovered, RGBA32::DarkGreen,
					ImGuiCol_ButtonActive, RGBA32::NiceGreen);
				if (ImGui::Button(LK_ICON_PLUS, ImVec2(IconButton, IconButton))) {
					const glm::vec2 InsertAt = ((Idx + 1) < State.Points.size())
						? (State.Points.at(Idx) + State.Points.at(Idx + 1)) * 0.50f
						: State.Points.at(Idx) + glm::vec2(0.50f, 0.0f);
					State.Points.insert(State.Points.begin() + Idx + 1, InsertAt);
					PointsModified = true;
					break;
				}
			}
			UI::SetTooltip("Insert point after");

			/* Delete. */
			ImGui::SameLine(0.0f, 4.0f);
			const bool CanDelete = (State.Points.size() > 4);
			if (!CanDelete) {
				ImGui::BeginDisabled();
			}
			{
				UI::FScopedColorStack Colors(
					ImGuiCol_Button, RGBA32::BackgroundDark,
					ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.12f, 0.12f, 0.85f),
					ImGuiCol_ButtonActive, ImVec4(0.70f, 0.15f, 0.15f, 1.0f));
				if (ImGui::Button(LK_ICON_TIMES, ImVec2(IconButton, IconButton))) {
					State.Points.erase(State.Points.begin() + Idx);
					PointsModified = true;
					break;
				}
			}
			if (!CanDelete) {
				ImGui::EndDisabled();
			}
			UI::SetTooltip("Delete point");
		}

		ImGui::Dummy(ImVec2(0, 4));
		{
			UI::FScopedColorStack Colors(
				ImGuiCol_Button, RGBA32::DarkGreen,
				ImGuiCol_ButtonHovered, RGBA32::NiceGreen,
				ImGuiCol_ButtonActive, RGBA32::LightGreen);
			std::array<char, 32> Label = {0};
			std::snprintf(Label.data(), Label.size(), "%s  Append", LK_ICON_PLUS_CIRCLE);
			if (ImGui::Button(Label.data())) {
				const glm::vec2 Tail = State.Points.empty()
					? glm::vec2(0.0f, 0.0f)
					: (State.Points.back() + glm::vec2(0.50f, 0.0f));
				State.Points.push_back(Tail);
				PointsModified = true;
			}
		}
		ImGui::SameLine();
		{
			std::array<char, 32> Label = {0};
			std::snprintf(Label.data(), Label.size(), "%s  Reset", LK_ICON_REFRESH);
			if (ImGui::Button(Label.data())) {
				State.ResetPoints();
				PointsModified = true;
			}
		}

		return PointsModified;
	}

	void RenderTerrainCreator(std::shared_ptr<CScene> Scene)
	{
		if (!TerrainCreator.bOpen) {
			return;
		}
		const bool Opened = UI::Begin(PanelID::TerrainCreator, &TerrainCreator.bOpen);
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
					State.TextureSide = Chain->TextureSide;
					State.TextureOffset = Chain->TextureOffset;
					State.bTextureTile = Chain->bTextureTile;
					State.TextureTileWidth = Chain->TextureTileWidth;
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

		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted("Editing");
		ImGui::SameLine();
		if (NewChain) {
			UI::FScopedColor TextColor(ImGuiCol_Text, RGBA32::NiceGreen);
			UI::FScopedFont Font(EFontModifier::Bold);
			ImGui::TextUnformatted("<New chain>"); /* @fixme: Don't really like this format */
		} else {
			{
				UI::FScopedColor TextColor(ImGuiCol_Text, RGBA32::NiceBlue);
				ImGui::Text("%s", std::string(EditActor->GetName()).c_str());
			}
			ImGui::SameLine();
			if (ImGui::SmallButton("Clear##ChainEdit")) {
				State.bHasEditTarget = false;
			}
		}

		ImGui::Dummy(ImVec2(0, 6));

		/* Section: Chain */
		if (TerrainSection("Chain")) {
			BeginPropertyGrid();
			Table::NextRow();
			Table::Label("Name");
			Table::NextColumn();
			{
				UI::FScopedStyle InputRounding(ImGuiStyleVar_FrameRounding, 6.0f);
				ImGui::SetNextItemWidth(-1.0f);
				ImGui::InputText("##NameBuf", State.NameBuf.data(), State.NameBuf.size());
			}

			Table::NextRow();
			UI::Checkbox("Preview", State.bPreviewVisible);
			EndPropertyGrid();

			ImGui::Dummy(ImVec2(0, 2));
			UI::ShiftCursorX(4.0f);
			FChipRow Row;
			if (UI::FlagChip("Loop", State.bLoop, RGBA32::NiceBlue)) {
				State.bLoop = !State.bLoop;
				if (EditActor) {
					ApplyChainToActor(EditActor, State);
				}
			}
			Row.Next("Block Both Sides", true);
			if (UI::FlagChip("Block Both Sides", State.bBlockBothSides, RGBA32::Orange)) {
				State.bBlockBothSides = !State.bBlockBothSides;
				if (EditActor) {
					ApplyChainToActor(EditActor, State);
				}
			}
			UI::SetTooltip("Off: one-way platform (stand on top, jump up through). On: solid wall");

			ImGui::TreePop();
		}

		/* Section: Points */
		bool PointsModified = false;
		if (TerrainSection("Points")) {
			if (NewChain) {
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted("Spawn at");
				ImGui::SameLine();
				ImGui::SetNextItemWidth(150.0f);
				ImGui::DragFloat2("##Origin", &State.PreviewOrigin.x, 0.05f, 0.0f, 0.0f, "%.2f");
				ImGui::SameLine();

				std::array<char, 32> Label = {0};
				std::snprintf(Label.data(), Label.size(), "%s  To Player", LK_ICON_LOCATION_ARROW);
				if (ImGui::Button(Label.data())) {
					if (auto Player = CGameInstance::Get().GetPlayer(0); Player != nullptr) {
						const glm::vec3 PlayerPos = Player->GetPosition();
						State.PreviewOrigin = {PlayerPos.x, PlayerPos.y};
					}
				}
			}

			ImGui::AlignTextToFramePadding();
			ImGui::Text("%zu points", State.Points.size());
			ImGui::SameLine();
			{
				std::array<char, 32> Label = {0};
				std::snprintf(Label.data(), Label.size(), "%s  Reverse", LK_ICON_EXCHANGE);
				if (ImGui::Button(Label.data())) {
					std::reverse(State.Points.begin(), State.Points.end());
					if (EditActor) {
						ApplyChainToActor(EditActor, State);
					}
				}
			}
			if (ImGui::IsItemHovered()) {
				UI::SetTooltip("Flip the side that collides for one-way chains");
			}

			ImGui::Dummy(ImVec2(0, 4));
			PointsModified = UI_ChainPoints();

			ImGui::TreePop();
		}

		/* Section: Texture */
		if (TerrainSection("Texture")) {
			BeginPropertyGrid();
			Table::NextRow();
			if (ColorDropdown(State.Color)) {
				if (SelectedActor) {
					SelectedActor->SetColor(FColor::Get(State.Color));
				}
			}

			Table::NextRow();
			if (UI::TextureDropdown(State.Texture)) {
				if (EditActor) {
					ApplyChainToActor(EditActor, State);
				}
			}

			Table::NextRow();
			if (UI::DragFloat("Texture Height", State.TextureHeight, 0.01f, 0.0f, 10.0f, "%.3f")) {
				if (EditActor) {
					ApplyChainToActor(EditActor, State);
				}
			}

			Table::NextRow();
			Table::Label("Texture Side");
			Table::NextColumn();
			ImGui::SetNextItemWidth(-1.0f);
			if (UI::Combo("##TextureSide", Enum::View<EDirection>(), State.TextureSide)) {
				if (EditActor) {
					ApplyChainToActor(EditActor, State);
				}
			}
			if (ImGui::IsItemHovered()) {
				UI::SetTooltip("None = centered on chain. Up/Down = offset perpendicular to segment. Left/Right unused for chains");
			}

			Table::NextRow();
			if (UI::DragFloat("Texture Offset", State.TextureOffset, 0.01f, -10.0f, 10.0f, "%.3f")) {
				if (EditActor) {
					ApplyChainToActor(EditActor, State);
				}
			}

			Table::NextRow();
			if (UI::DragFloat("Tile Width", State.TextureTileWidth, 0.01f, 0.0f, 10.0f, "%.3f")) {
				if (EditActor) {
					ApplyChainToActor(EditActor, State);
				}
			}
			EndPropertyGrid();

			ImGui::Dummy(ImVec2(0, 2));
			UI::ShiftCursorX(4.0f);
			if (UI::FlagChip("Tile Texture", State.bTextureTile, RGBA32::Orange)) {
				State.bTextureTile = !State.bTextureTile;
				if (EditActor) {
					ApplyChainToActor(EditActor, State);
				}
			}

			ImGui::TreePop();
		}

		UI_DebugInfo();
		ImGui::Dummy(ImVec2(0, 12));

		const bool ValidPoints = (State.Points.size() >= 4);
		if (!ValidPoints) {
			ImGui::TextColored(FColor::Convert<ImVec4>(RGBA32::Red), "Need at least 4 points");
			ImGui::BeginDisabled();
		}
		{
			UI::FScopedStyle ButtonPad(ImGuiStyleVar_FramePadding, ImVec2(6, 8));
			UI::FScopedColorStack ButtonColors(
				ImGuiCol_Button, RGBA32::DarkGreen,
				ImGuiCol_ButtonHovered, RGBA32::NiceGreen,
				ImGuiCol_ButtonActive, RGBA32::LightGreen);
			if (EditActor) {
				if (ImGui::Button("Apply", ImVec2(-1.0f, 0.0f)) || (PointsModified && ValidPoints)) {
					ApplyChainToActor(EditActor, State);
				}
			} else {
				std::array<char, 32> Label = {0};
				std::snprintf(Label.data(), Label.size(), "%s  Create Chain", LK_ICON_PLUS_CIRCLE);
				if (ImGui::Button(Label.data(), ImVec2(-1.0f, 0.0f))) {
					std::shared_ptr<CActor> Actor = CSpawner::CreateChain(State.NameBuf.data(), State.Points, State.bLoop, State.bBlockBothSides, FColor::Get(State.Color));
					if (Actor) {
						Actor->SetPosition(State.PreviewOrigin);
						State.EditTarget = Actor->GetHandle();
						State.bHasEditTarget = true;
					}
				}
			}
		}
		if (!ValidPoints) {
			ImGui::EndDisabled();
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
