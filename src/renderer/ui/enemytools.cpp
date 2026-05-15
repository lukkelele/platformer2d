#include "enemytools.h"

#include "core/log.h"
#include "core/selectioncontext.h"
#include "core/string.h"
#include "game/controller/patrolcontroller.h"
#include "game/enemy.h"
#include "renderer/color.h"
#include "renderer/fontawesome.h"
#include "renderer/font.h"
#include "renderer/texture.h"
#include "physics/body.h"
#include "physics/bodytype.h"
#include "scene/components.h"
#include "scene/scene.h"
#include "combo.h"
#include "ui_core.h"
#include "ui.h"

namespace platformer2d::UI {

	static std::shared_ptr<CEnemy> GetSelectedEnemy(const std::shared_ptr<CScene>& Scene)
	{
		if (!Scene || !CSelectionContext::IsAnySelected()) {
			return nullptr;
		}
		std::shared_ptr<CActor> Selected = Scene->GetActor(CSelectionContext::GetSelected());
		if (!Selected) {
			return nullptr;
		}
		return std::dynamic_pointer_cast<CEnemy>(Selected);
	}

	static std::string NextEnemyName(const std::shared_ptr<CScene>& Scene, const EEnemyArchetype Archetype)
	{
		const std::string Prefix = Format("Enemy-{}", Enum::ToString(Archetype));
		for (std::size_t Idx = 1; Idx < 10000; Idx++) {
			std::string Candidate = Format("{}-{}", Prefix, Idx);
			if (!Scene->DoesActorExist(Candidate)) {
				return Candidate;
			}
		}
		return Format("{}-{}", Prefix, std::rand());
	}

	static void SpawnArchetype(const std::shared_ptr<CScene>& Scene, const EEnemyArchetype Archetype, const glm::vec2& Pos)
	{
		FEnemySpecification EnemySpec;
		EnemySpec.Archetype = Archetype;
		EnemySpec.SpawnPoint = Pos;

		FActorSpecification ActorSpec;
		ActorSpec.Name = NextEnemyName(Scene, Archetype);
		ActorSpec.Texture = ETexture::Goblin;

		FBodySpecification BodySpec;
		BodySpec.Type = EBodyType::Dynamic;
		BodySpec.Position = Pos;
		BodySpec.Flags = EBodyFlag_PreSolveEvents;
		BodySpec.Shape.emplace<FPolygon>(FPolygon{.Size = glm::vec2(0.20f, 0.20f)});

		LK_INFO_TAG("EnemyTools", "Spawn {} at ({:.2f}, {:.2f})", Enum::ToString(Archetype), Pos.x, Pos.y);
		std::shared_ptr<CEnemy> Enemy = Scene->Create<CEnemy>(EnemySpec, ActorSpec, BodySpec);
		Enemy->SetController(std::make_unique<CPatrolController>(1.0f, 1.0f));
		Enemy->AddComponent<FHealthComponent>();
	}

	static void DrawSpawnRow(const std::shared_ptr<CScene>& Scene)
	{
		constexpr ImVec2 ButtonSize = ImVec2(96.0f, 36.0f);
		UI::FScopedStyle ButtonRounding(ImGuiStyleVar_FrameRounding, 6.0f);

		constexpr glm::vec2 SpawnPos = {0.0f, 0.0f};
		if (ImGui::Button(LK_ICON_USER_SECRET "  Grunt", ButtonSize)) {
			SpawnArchetype(Scene, EEnemyArchetype::Grunt, SpawnPos);
		}
		ImGui::SameLine();
		if (ImGui::Button(LK_ICON_USER_MD "  Jumper", ButtonSize)) {
			SpawnArchetype(Scene, EEnemyArchetype::Jumper, SpawnPos);
		}
		ImGui::SameLine();
		if (ImGui::Button(LK_ICON_CROSSHAIRS "  Ranged", ButtonSize)) {
			SpawnArchetype(Scene, EEnemyArchetype::RangedShooter, SpawnPos);
		}
	}

	static void DrawBulkActions(const std::shared_ptr<CScene>& Scene)
	{
		constexpr ImVec2 ButtonSize = ImVec2(120.0f, 32.0f);
		UI::FScopedStyle ButtonRounding(ImGuiStyleVar_FrameRounding, 6.0f);

		{
			UI::FScopedColor ButtonBg(ImGuiCol_Button, IM_COL32(180, 45, 45, 200));
			UI::FScopedColor ButtonHover(ImGuiCol_ButtonHovered, IM_COL32(220, 60, 60, 220));
			if (ImGui::Button(LK_ICON_TIMES "  Kill All", ButtonSize)) {
				std::vector<std::shared_ptr<CEnemy>> Enemies = Scene->GetAllOfType<CEnemy>();
				LK_INFO_TAG("EnemyTools", "Killing {} enemies", Enemies.size());
				for (const std::shared_ptr<CEnemy>& Enemy : Enemies) {
					if (Enemy && !Enemy->IsDead()) {
						Enemy->Kill();
					}
				}
			}
		}

		ImGui::SameLine();
		{
			UI::FScopedColor ButtonBg(ImGuiCol_Button, RGBA32::SmoothGreen);
			UI::FScopedColor ButtonHover(ImGuiCol_ButtonHovered, IM_COL32(0, 205, 15, 90));
			if (ImGui::Button(LK_ICON_HEART "  Revive All", ButtonSize)) {
				std::vector<std::shared_ptr<CEnemy>> Enemies = Scene->GetAllOfType<CEnemy>();
				LK_INFO_TAG("EnemyTools", "Reviving {} enemies", Enemies.size());
				for (const std::shared_ptr<CEnemy>& Enemy : Enemies) {
					if (Enemy && Enemy->IsDead()) {
						Enemy->Revive(CEnemy::EReviveVariant::AtSpawn);
					}
				}
			}
		}
	}

	static void DrawSelectedSection(const std::shared_ptr<CEnemy>& Enemy)
	{
		ImGui::Dummy(ImVec2(0.0f, 4.0f));
		{
			UI::FScopedColor SubHeader(ImGuiCol_Text, RGBA32::Text::Darker);
			UI::FScopedFont SubFont(EFont::Roboto, EFontSize::Regular, EFontModifier::Bold);
			ImGui::TextUnformatted("Selected");
		}

		if (!Enemy) {
			UI::FScopedColor TextColor(ImGuiCol_Text, RGBA32::Text::Darker);
			ImGui::TextUnformatted("(no enemy selected)");
			return;
		}

		BeginPropertyGrid(120.0f);

		Table::NextRow();
		Table::Label("Name");
		Table::NextColumn();
		ImGui::Text("%s", Enemy->GetName().data());

		Table::NextRow();
		Table::Label("Archetype");
		Table::NextColumn();
		ImGui::Text("%s", Enum::ToString(Enemy->GetArchetype()));

		Table::NextRow();
		Table::Label("State");
		Table::NextColumn();
		EEnemyState State = Enemy->GetState();
		if (UI::Combo("##EnemyState", Enum::View<EEnemyState>(), State)) {
			Enemy->SetState(State);
		}

		Table::NextRow();
		Table::Label("Dead");
		Table::NextColumn();
		ImGui::Text("%s", Enemy->IsDead() ? "Yes" : "No");

		EndPropertyGrid();

		ImGui::Dummy(ImVec2(0.0f, 4.0f));
		constexpr ImVec2 ButtonSize = ImVec2(100.0f, 32.0f);
		UI::FScopedStyle ButtonRounding(ImGuiStyleVar_FrameRounding, 6.0f);
		const bool Dead = Enemy->IsDead();

		if (Dead) {
			ImGui::BeginDisabled();
		}
		{
			UI::FScopedColor ButtonBg(ImGuiCol_Button, IM_COL32(180, 45, 45, 200));
			UI::FScopedColor ButtonHover(ImGuiCol_ButtonHovered, IM_COL32(220, 60, 60, 220));
			if (ImGui::Button(LK_ICON_TIMES "  Kill", ButtonSize)) {
				Enemy->Kill();
			}
		}
		if (Dead) {
			ImGui::EndDisabled();
		}

		ImGui::SameLine();
		if (!Dead) {
			ImGui::BeginDisabled();
		}
		{
			UI::FScopedColor ButtonBg(ImGuiCol_Button, RGBA32::SmoothGreen);
			UI::FScopedColor ButtonHover(ImGuiCol_ButtonHovered, IM_COL32(0, 205, 15, 90));
			if (ImGui::Button(LK_ICON_HEART "  Revive", ButtonSize)) {
				Enemy->Revive(CEnemy::EReviveVariant::AtSpawn);
			}
		}
		if (!Dead) {
			ImGui::EndDisabled();
		}
	}

	void EnemyTools(const std::shared_ptr<CScene>& Scene)
	{
		UI::Font::Push(EFont::SourceSansPro, EFontSize::Large, EFontModifier::Bold);
		const bool TreeOpen = ImGui::TreeNodeEx("Enemies", ImGuiTreeNodeFlags_SpanAvailWidth);
		UI::Font::Pop();
		if (!TreeOpen) {
			return;
		}

		if (!Scene) {
			UI::FScopedColor TextColor(ImGuiCol_Text, RGBA32::Text::Darker);
			ImGui::TextUnformatted("No scene loaded");
			ImGui::TreePop();
			return;
		}

		{
			UI::FScopedColor SubHeader(ImGuiCol_Text, RGBA32::Text::Darker);
			UI::FScopedFont SubFont(EFont::Roboto, EFontSize::Regular, EFontModifier::Bold);
			ImGui::TextUnformatted("Spawn");
		}
		DrawSpawnRow(Scene);

		ImGui::Dummy(ImVec2(0.0f, 6.0f));
		{
			UI::FScopedColor SubHeader(ImGuiCol_Text, RGBA32::Text::Darker);
			UI::FScopedFont SubFont(EFont::Roboto, EFontSize::Regular, EFontModifier::Bold);
			ImGui::TextUnformatted("Bulk");
		}
		DrawBulkActions(Scene);

		std::shared_ptr<CEnemy> Selected = GetSelectedEnemy(Scene);
		DrawSelectedSection(Selected);

		ImGui::TreePop();
	}

}
