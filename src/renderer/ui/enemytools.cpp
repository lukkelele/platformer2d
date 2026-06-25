#include "enemytools.h"

#include <unordered_set>

#include "core/log.h"
#include "core/selectioncontext.h"
#include "core/string.h"
#include "game/controller/patrolcontroller.h"
#include "game/enemy.h"
#include "game/enemyspawner.h"
#include "renderer/color.h"
#include "renderer/fontawesome.h"
#include "renderer/font.h"
#include "renderer/renderer.h"
#include "renderer/texture.h"
#include "physics/body.h"
#include "physics/bodytype.h"
#include "scene/components.h"
#include "scene/scene.h"
#include "combo.h"
#include "ui_core.h"
#include "ui.h"

namespace platformer2d::UI {

	static bool bShowAllSpawnPoints = false;
	static std::unordered_set<LUUID> VisibleSpawnPoints;

	bool IsSpawnPointGloballyVisible()
	{
		return bShowAllSpawnPoints;
	}

	void SetSpawnPointGloballyVisible(const bool Enabled)
	{
		bShowAllSpawnPoints = Enabled;
	}

	bool IsSpawnPointVisible(const LUUID Handle)
	{
		return VisibleSpawnPoints.contains(Handle);
	}

	void SetSpawnPointVisible(const LUUID Handle, const bool Visible)
	{
		if (Visible) {
			VisibleSpawnPoints.insert(Handle);
		} else {
			VisibleSpawnPoints.erase(Handle);
		}
	}

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
		BodySpec.Flags = EBodyFlag_PreSolveEvents | EBodyFlag_ContactEvents;
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
			UI::FScopedColor ButtonBg(ImGuiCol_Button, FColor::DarkRed.As<std::uint32_t>());
			UI::FScopedColor ButtonHover(ImGuiCol_ButtonHovered, FColor::WarmRed.As<std::uint32_t>());
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
			UI::FScopedColor ButtonHover(ImGuiCol_ButtonHovered, FColor::VividGreen.As<std::uint32_t>());
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

		ImGui::Dummy(ImVec2(0.0f, 4.0f));

		static EEnemyState BulkState = EEnemyState::Idle;
		ImGui::SetNextItemWidth(140.0f);
		UI::Combo("##BulkEnemyState", Enum::View<EEnemyState>(), BulkState);
		ImGui::SameLine(0.0f, 8.0f);
		{
			UI::FScopedColor ButtonBg(ImGuiCol_Button, FColor::RoyalBlue.As<std::uint32_t>());
			UI::FScopedColor ButtonHover(ImGuiCol_ButtonHovered, FColor::HoverBlue.As<std::uint32_t>());
			if (ImGui::Button(LK_ICON_BOLT "  Set State All", ButtonSize)) {
				std::vector<std::shared_ptr<CEnemy>> Enemies = Scene->GetAllOfType<CEnemy>();
				LK_INFO_TAG("EnemyTools", "Set state of {} enemies to {}", Enemies.size(), Enum::ToString(BulkState));
				for (const std::shared_ptr<CEnemy>& Enemy : Enemies) {
					if (Enemy) {
						Enemy->SetState(BulkState);
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
		ImGui::Text("%s", Enum::ToString<const char*>(Enemy->GetArchetype()));

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
			UI::FScopedColor ButtonBg(ImGuiCol_Button, FColor::DarkRed.As<std::uint32_t>());
			UI::FScopedColor ButtonHover(ImGuiCol_ButtonHovered, FColor::WarmRed.As<std::uint32_t>());
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
			UI::FScopedColor ButtonHover(ImGuiCol_ButtonHovered, FColor::VividGreen.As<std::uint32_t>());
			if (ImGui::Button(LK_ICON_HEART "  Revive", ButtonSize)) {
				Enemy->Revive(CEnemy::EReviveVariant::AtSpawn);
			}
		}
		if (!Dead) {
			ImGui::EndDisabled();
		}
	}

	static std::shared_ptr<CEnemySpawner> GetSelectedSpawner(const std::shared_ptr<CScene>& Scene)
	{
		if (!Scene || !CSelectionContext::IsAnySelected()) {
			return nullptr;
		}
		std::shared_ptr<CActor> Selected = Scene->GetActor(CSelectionContext::GetSelected());
		if (!Selected) {
			return nullptr;
		}
		return std::dynamic_pointer_cast<CEnemySpawner>(Selected);
	}

	static void DrawWaveEditor(const std::shared_ptr<CEnemySpawner>& Spawner)
	{
		ImGui::Dummy(ImVec2(0.0f, 4.0f));
		{
			UI::FScopedColor SubHeader(ImGuiCol_Text, RGBA32::Text::Darker);
			UI::FScopedFont SubFont(EFont::Roboto, EFontSize::Regular, EFontModifier::Bold);
			ImGui::TextUnformatted("Waves");
		}

		std::vector<FSpawnWave>& Waves = Spawner->GetWaves();
		int RemoveIdx = -1;
		for (std::size_t Idx = 0; Idx < Waves.size(); Idx++) {
			FSpawnWave& Wave = Waves.at(Idx);
			UI::FScopedID WaveID(static_cast<int>(Idx));

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Wave %zu", Idx + 1);
			ImGui::SameLine();
			{
				UI::FScopedColor ButtonBg(ImGuiCol_Button, FColor::DarkRed.As<std::uint32_t>());
				UI::FScopedColor ButtonHover(ImGuiCol_ButtonHovered, FColor::WarmRed.As<std::uint32_t>());
				if (ImGui::SmallButton(LK_ICON_TIMES)) {
					RemoveIdx = static_cast<int>(Idx);
				}
			}

			BeginPropertyGrid(120.0f);

			Table::NextRow();
			Table::Label("Archetype");
			Table::NextColumn();
			UI::Combo("##WaveArchetype", Enum::View<EEnemyArchetype>(), Wave.Archetype);

			Table::NextRow();
			Table::Label("Count");
			Table::NextColumn();
			ImGui::SetNextItemWidth(-1.0f);
			ImGui::DragInt("##WaveCount", &Wave.Count, 1.0f, 1, 100);

			Table::NextRow();
			Table::Label("Interval");
			Table::NextColumn();
			ImGui::SetNextItemWidth(-1.0f);
			ImGui::DragFloat("##WaveInterval", &Wave.SpawnInterval, 0.01f, 0.0f, 10.0f);

			Table::NextRow();
			Table::Label("Start Delay");
			Table::NextColumn();
			ImGui::SetNextItemWidth(-1.0f);
			ImGui::DragFloat("##WaveDelay", &Wave.StartDelay, 0.01f, 0.0f, 10.0f);

			Table::NextRow();
			Table::Label("Wait For Clear");
			Table::NextColumn();
			ImGui::Checkbox("##WaveWait", &Wave.bWaitForClear);

			EndPropertyGrid();
			ImGui::Dummy(ImVec2(0.0f, 2.0f));
		}

		if (RemoveIdx >= 0) {
			Waves.erase(Waves.begin() + RemoveIdx);
		}

		if (ImGui::Button(LK_ICON_PLUS "  Add Wave")) {
			Waves.push_back(FSpawnWave{});
		}
	}

	static void DrawSelectedSpawnerSection(const std::shared_ptr<CEnemySpawner>& Spawner)
	{
		if (!Spawner) {
			return;
		}

		ImGui::Dummy(ImVec2(0.0f, 6.0f));
		{
			UI::FScopedColor SubHeader(ImGuiCol_Text, RGBA32::Text::Darker);
			UI::FScopedFont SubFont(EFont::Roboto, EFontSize::Regular, EFontModifier::Bold);
			ImGui::TextUnformatted("Spawner");
		}

		BeginPropertyGrid(120.0f);

		Table::NextRow();
		Table::Label("Name");
		Table::NextColumn();
		ImGui::Text("%s", Spawner->GetName().data());

		Table::NextRow();
		Table::Label("Status");
		Table::NextColumn();
		ImGui::Text("%s", Spawner->IsActive() ? "Active" : (Spawner->IsFinished() ? "Finished" : "Idle"));

		Table::NextRow();
		Table::Label("Max Alive");
		Table::NextColumn();
		int MaxAlive = Spawner->GetMaxAlive();
		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::DragInt("##MaxAlive", &MaxAlive, 1.0f, 1, 64)) {
			Spawner->SetMaxAlive(MaxAlive);
		}

		Table::NextRow();
		Table::Label("Scatter");
		Table::NextColumn();
		float Scatter = Spawner->GetScatter();
		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::DragFloat("##Scatter", &Scatter, 0.01f, 0.0f, 4.0f)) {
			Spawner->SetScatter(Scatter);
		}

		Table::NextRow();
		Table::Label("Loop");
		Table::NextColumn();
		bool Loop = Spawner->IsLooping();
		if (ImGui::Checkbox("##Loop", &Loop)) {
			Spawner->SetLoop(Loop);
		}

		Table::NextRow();
		Table::Label("Auto Activate");
		Table::NextColumn();
		bool Auto = Spawner->IsAutoActivate();
		if (ImGui::Checkbox("##AutoActivate", &Auto)) {
			Spawner->SetAutoActivate(Auto);
		}

		EndPropertyGrid();

		DrawWaveEditor(Spawner);

		ImGui::Dummy(ImVec2(0.0f, 6.0f));
		constexpr ImVec2 ButtonSize = ImVec2(100.0f, 30.0f);
		UI::FScopedStyle ButtonRounding(ImGuiStyleVar_FrameRounding, 6.0f);
		{
			UI::FScopedColor ButtonBg(ImGuiCol_Button, RGBA32::SmoothGreen);
			UI::FScopedColor ButtonHover(ImGuiCol_ButtonHovered, FColor::VividGreen.As<std::uint32_t>());
			if (ImGui::Button(LK_ICON_PLAY "  Activate", ButtonSize)) {
				Spawner->Activate();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button(LK_ICON_PAUSE "  Deactivate", ButtonSize)) {
			Spawner->Deactivate();
		}
		ImGui::SameLine();
		if (ImGui::Button(LK_ICON_UNDO "  Reset", ButtonSize)) {
			Spawner->Reset();
		}
	}

	void RenderEnemySpawnPoints(const std::shared_ptr<CScene>& Scene)
	{
		if (!Scene) {
			return;
		}
		const std::vector<std::shared_ptr<CEnemy>> Enemies = Scene->GetAllOfType<CEnemy>();
		if (Enemies.empty()) {
			return;
		}

		constexpr float MarkerRadius = 0.030f;
		constexpr float CrossArm = 0.130f;
		constexpr std::uint16_t LineWidth = 2;
		static const glm::vec4 MarkColor = FColor::Red.WithAlpha(0.95f);
		static const glm::vec4 LinkColor = FColor::Black.WithAlpha(0.95f);

		for (const std::shared_ptr<CEnemy>& Enemy : Enemies) {
			LK_ASSERT(Enemy);
			if (!bShowAllSpawnPoints && !VisibleSpawnPoints.contains(Enemy->GetHandle())) {
				continue;
			}

			const glm::vec2 Sp = Enemy->GetSpawnPoint();
			CRenderer::DrawCrossMark(Sp, MarkColor);

			const glm::vec3 EnemyPos = Enemy->GetPosition();
			CRenderer::DrawLine(EnemyPos, {Sp.x, Sp.y, 0.0f}, LinkColor, LineWidth);
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

		ImGui::Dummy(ImVec2(0.0f, 6.0f));
		{
			UI::FScopedColor SubHeader(ImGuiCol_Text, RGBA32::Text::Darker);
			UI::FScopedFont SubFont(EFont::Roboto, EFontSize::Regular, EFontModifier::Bold);
			ImGui::TextUnformatted("Visualization");
		}
		ImGui::Checkbox("Show All Spawn Points", &bShowAllSpawnPoints);

		std::shared_ptr<CEnemy> Selected = GetSelectedEnemy(Scene);
		DrawSelectedSection(Selected);

		std::shared_ptr<CEnemySpawner> SelectedSpawner = GetSelectedSpawner(Scene);
		DrawSelectedSpawnerSection(SelectedSpawner);

		ImGui::TreePop();
	}

}
