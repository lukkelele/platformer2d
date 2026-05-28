#include "creatorpanel.h"

#include "core/log.h"
#include "core/selectioncontext.h"
#include "game/gameplaysystem.h"
#include "game/instance.h"
#include "game/player.h"
#include "game/spawner.h"
#include "physics/body.h"
#include "renderer/font.h"
#include "renderer/renderer.h"
#include "renderer/ui/physics.h"
#include "renderer/ui/quickcreator.h"
#include "renderer/ui/ui.h"
#include "renderer/ui/ui_core.h"
#include "renderer/ui/widgets.h"
#include "scene/scene.h"

namespace platformer2d::UI {

	FActorAttributes ActorAttr;

	bool ActorAttributes(FActorAttributes& Attr)
	{
		bool Updated = false;
		constexpr float COL_WIDTH = 180.0f;
		BeginPropertyGrid(COL_WIDTH);

		Table::NextRow();
		Table::Label("Name");
		Table::NextColumn();
		{
			UI::FScopedStyle InputRounding(ImGuiStyleVar_FrameRounding, 6.0f);
			ImGui::SetNextItemWidth(-1.0f);
			ImGui::InputText("##ActorName", Attr.NameBuf.data(), Attr.NameBuf.size());
		}

		Table::NextRow();
		Updated |= UI::DragFloat2("Position", Attr.Position, 0.0f, 0.010f, -100.0f, 100.0f);

		Table::NextRow();
		Updated |= UI::DragFloat2("Size", Attr.Size, 1.0f, 0.010f, 0.010f, 2.0f);

		Table::NextRow();
		Updated |= UI::TextureDropdown(Attr.Texture);

		Table::NextRow();
		Updated |= ColorDropdown(Attr.Color);

		EndPropertyGrid();

		return Updated;
	}

	void Creator(std::shared_ptr<CScene>& Scene)
	{
		const bool WindowOpened = UI::Begin(PanelID::Creator, nullptr);
		if (!WindowOpened) {
			return;
		}
		if (!Scene) {
			UI::End();
			return;
		}

		static const std::string FuncID = LK_FUNCSIG;
		ImGui::PushID(FuncID.c_str());

		QuickCreator(Scene);
		ImGui::Dummy(ImVec2(0, 20));
		UI::Separator(2);

		{
			UI::FScopedStyle FramePadding(ImGuiStyleVar_FramePadding, ImVec2(6, 3));
			UI::FScopedStyle FrameRounding(ImGuiStyleVar_FrameRounding, 6.0f);

			{
				UI::FScopedFont HeaderFont(EFont::SourceSansPro, EFontSize::Large, EFontModifier::Bold);
				ImGui::Text("%s  Actor", LK_ICON_CUBE);
			}
			ImGui::Dummy(ImVec2(0, 2));

			BeginPropertyGrid();
			Table::NextRow();
			Checkbox("Preview", ActorAttr.bPreview);
			EndPropertyGrid();

			ActorAttributes(ActorAttr);

			/* Menu: Physics body */
			ImGui::Dummy(ImVec2(0, 14));
			Font::Push(EFont::SourceSansPro, EFontSize::Large, EFontModifier::Bold);
			ImGui::Text("%s  Physics", LK_ICON_PENCIL);
			Font::Pop();
			ImGui::Dummy(ImVec2(0, 2));
			PhysicsBodyMenu(PhysicsBodyData);

			ImGui::Dummy(ImVec2(0, 18));
			ActorCreateButtons(Scene);
		}

		ImGui::PopID();
		UI::End();
	}

	void ActorCreateButtons(std::shared_ptr<CScene>& Scene)
	{
		LK_ASSERT(Scene);
		const ImVec2 Avail = ImGui::GetContentRegionAvail();
		constexpr ImVec2 ButtonSize = ImVec2(112, 50);

		ImGui::BeginTable("##ActorButtons", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip);
		ImGui::TableSetupColumn("L", 0, Avail.x * 0.30f);
		ImGui::TableSetupColumn("V", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, Avail.x * 0.60);

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(1);

		/* Button: Create */
		UI::FScopedFont Font(UI::Font::Get(EFont::SourceSansPro, EFontSize::Large, EFontModifier::Bold));
		UI::FScopedStyle ButtonFrame(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
		UI::FScopedStyle ButtonRounding(ImGuiStyleVar_FrameRounding, 8);
		{
			UI::FScopedColorStack ButtonColours(
				ImGuiCol_Button, RGBA32::LightGreen,
				ImGuiCol_ButtonHovered, RGBA32::DarkGreen,
				ImGuiCol_ButtonActive, RGBA32::NiceGreen);

			const bool ActorExists = Scene->DoesActorExist(ActorAttr.NameBuf.data());
			if (ActorExists) {
				ImGui::BeginDisabled();
			}

			UI::ShiftCursorX((0.50f * ImGui::GetContentRegionAvail().x) - (0.50f * ButtonSize.x));
			if (ImGui::Button("Create", ButtonSize)) {
				FBodySpecification NewBodySpec;
				Aggregate(PhysicsBodyData, NewBodySpec);
				LK_INFO_TAG("Creator", "{}", CBody::ToString(NewBodySpec));

				auto& GameInstance = CGameInstance::Get();
				CGameplaySystem& Gameplay = GameInstance.GetSystem<CGameplaySystem>();
				if (NewBodySpec.Type == EBodyType::Static) {
					std::shared_ptr<CActor> SpawnedPolygon = CSpawner::CreateStaticPolygon(
						ActorAttr.NameBuf.data(),
						ActorAttr.Position,
						ActorAttr.Size,
						NewBodySpec,
						FColor::Get(ActorAttr.Color));

					/* @fixme: Only do this if the preview is disabled and not within range. */
					if (!ActorAttr.bPreview) {
						const glm::vec3 PlayerPos = GameInstance.GetPlayer(0)->GetPosition();
						Gameplay.Teleport(SpawnedPolygon, {PlayerPos.x, PlayerPos.y + 0.50f});
					}
				} else if (NewBodySpec.Type == EBodyType::Dynamic) {
					std::shared_ptr<CActor> SpawnedPolygon = CSpawner::CreatePolygon(
						ActorAttr.NameBuf.data(),
						NewBodySpec,
						ActorAttr.Size,
						FColor::Get(ActorAttr.Color),
						ActorAttr.Texture);

					/* @fixme: Only do this if the preview is disabled and not within range. */
					if (!ActorAttr.bPreview) {
						const glm::vec3 PlayerPos = GameInstance.GetPlayer(0)->GetPosition();
						Gameplay.Teleport(SpawnedPolygon, {PlayerPos.x, PlayerPos.y + 0.50f});
					}
				}
			}

			if (ActorExists) {
				ImGui::EndDisabled();
			}
		}

		ImGui::EndTable();
	}

	void RenderActorPreview(const std::shared_ptr<CScene>& Scene)
	{
		if (!ActorAttr.bPreview || !Scene) {
			return;
		}

		const glm::vec2 Pos = ActorAttr.Position;
		const glm::vec2 Size = ActorAttr.Size;
		if ((Size.x <= 0.0f) || (Size.y <= 0.0f)) {
			return;
		}

		const glm::vec4 BaseColor = FColor::Get(ActorAttr.Color);
		const glm::vec4 FillColor(BaseColor.r, BaseColor.g, BaseColor.b, 0.55f);
		const glm::vec4 OutlineColor(BaseColor.r, BaseColor.g, BaseColor.b, 0.95f);

		const glm::vec3 PreviewPos = {Pos.x, Pos.y, 0.0f};
		if (ActorAttr.Texture == ETexture::White) {
			CRenderer::DrawQuad(Pos, Size, FillColor);
		} else {
			CRenderer::DrawQuad(PreviewPos, Size, ActorAttr.Texture, FillColor);
		}

		const glm::vec2 Half = Size * 0.50f;
		const glm::vec3 TL = {Pos.x - Half.x, Pos.y + Half.y, 0.0f};
		const glm::vec3 TR = {Pos.x + Half.x, Pos.y + Half.y, 0.0f};
		const glm::vec3 BR = {Pos.x + Half.x, Pos.y - Half.y, 0.0f};
		const glm::vec3 BL = {Pos.x - Half.x, Pos.y - Half.y, 0.0f};

		const std::uint16_t OutlineWidth = ActorAttr.bPreviewSelected ? 4 : 2;
		const glm::vec4 BaseBodyColor = (PhysicsBodyData.BodyType == EBodyType::Static)
			? glm::vec4(0.30f, 0.85f, 1.0f, 0.85f)
			: glm::vec4(1.0f, 0.65f, 0.20f, 0.85f);
		const glm::vec4 BodyColor = ActorAttr.bPreviewSelected
			? glm::vec4(1.0f, 1.0f, 0.40f, 0.95f)
			: BaseBodyColor;

		CRenderer::DrawLine(TL, TR, BodyColor, OutlineWidth);
		CRenderer::DrawLine(TR, BR, BodyColor, OutlineWidth);
		CRenderer::DrawLine(BR, BL, BodyColor, OutlineWidth);
		CRenderer::DrawLine(BL, TL, BodyColor, OutlineWidth);
	}

	void RenderSelectedColliderPreview(const std::shared_ptr<CScene>& Scene)
	{
		if (!Scene) {
			return;
		}
		const LUUID Selected = CSelectionContext::GetSelected();
		if (Selected == 0) {
			return;
		}

		std::shared_ptr<CActor> Actor = Scene->GetActor(Selected);
		if (!Actor) {
			return;
		}
		const CBody* Body = Actor->GetBody();
		if (!Body) {
			return;
		}
		if (!Body->TryGetShape<EShape::Polygon>() && !Body->TryGetShape<EShape::Capsule>()) {
			return;
		}

		const glm::vec2 Center = Body->GetPosition();
		const glm::vec2 Size = Body->GetSize();
		if ((Size.x <= 0.0f) || (Size.y <= 0.0f)) {
			return;
		}

		const float Angle = Body->GetRotation();
		const float Cos = std::cos(Angle);
		const float Sin = std::sin(Angle);
		const glm::vec2 Half = Size * 0.50f;

		const auto Corner = [&](const float LocalX, const float LocalY) -> glm::vec3
		{
			return glm::vec3(
				Center.x + (LocalX * Cos - LocalY * Sin),
				Center.y + (LocalX * Sin + LocalY * Cos),
				0.0f);
		};

		const glm::vec3 TL = Corner(-Half.x, Half.y);
		const glm::vec3 TR = Corner(Half.x, Half.y);
		const glm::vec3 BR = Corner(Half.x, -Half.y);
		const glm::vec3 BL = Corner(-Half.x, -Half.y);

		constexpr std::uint16_t LineWidth = 5;
		const glm::vec4 Color = FColor::Yellow;
		CRenderer::DrawLine(TL, TR, Color, LineWidth);
		CRenderer::DrawLine(TR, BR, Color, LineWidth);
		CRenderer::DrawLine(BR, BL, Color, LineWidth);
		CRenderer::DrawLine(BL, TL, Color, LineWidth);
	}

}
