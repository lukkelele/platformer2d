#include "quickcreator.h"

#include <array>

#include "core/log.h"
#include "core/string.h"
#include "game/controller/patrolcontroller.h"
#include "game/enemy.h"
#include "game/spawner.h"
#include "renderer/color.h"
#include "renderer/fontawesome.h"
#include "renderer/font.h"
#include "renderer/texture.h"
#include "physics/body.h"
#include "physics/bodytype.h"
#include "scene/components.h"
#include "scene/scene.h"
#include "creatorpanel.h"
#include "ui_core.h"
#include "ui.h"

namespace platformer2d::UI {

	namespace {
		enum class EPresetAction : std::uint8_t
		{
			SpawnBlock,
			SpawnEnemy,
			SpawnSpawnpoint,
			COUNT
		};
		LK_ENUM(EPresetAction);

		enum class EPresetGroup : std::uint8_t
		{
			Blocks,
			Enemies,
			Misc,
			COUNT
		};
		LK_ENUM(EPresetGroup);

		struct FQuickPreset
		{
			EPresetGroup Group = EPresetGroup::Blocks;
			EPresetAction Action = EPresetAction::SpawnBlock;
			const char* Icon = nullptr;
			const char* Label = nullptr;

			glm::vec2 Size = {0.20f, 0.20f};
			glm::vec4 Color = FColor::White;
			ETexture Texture = ETexture::White;
			EBodyType BodyType = EBodyType::Static;

			EEnemyArchetype Archetype = EEnemyArchetype::Grunt;
		};
	}

	static const std::array<FQuickPreset, 9> Presets = {
		{
         {EPresetGroup::Blocks, EPresetAction::SpawnBlock, LK_ICON_CUBE, "Small", {0.20f, 0.20f}, FColor::Convert(RGBA32::Gray), ETexture::White, EBodyType::Static},
         {EPresetGroup::Blocks, EPresetAction::SpawnBlock, LK_ICON_CUBE, "Medium", {0.50f, 0.50f}, FColor::Convert(RGBA32::DarkerGray), ETexture::White, EBodyType::Static},
         {EPresetGroup::Blocks, EPresetAction::SpawnBlock, LK_ICON_CUBE, "Large", {1.0f, 0.30f}, FColor::Convert(RGBA32::Brown), ETexture::White, EBodyType::Static},
         {EPresetGroup::Blocks, EPresetAction::SpawnBlock, LK_ICON_CUBE, "Crate", {0.30f, 0.30f}, FColor::Convert(RGBA32::Purple), ETexture::White, EBodyType::Dynamic},

         {EPresetGroup::Enemies, EPresetAction::SpawnEnemy, LK_ICON_USER_SECRET, "Grunt", {0.20f, 0.20f}, FColor::White, ETexture::Goblin, EBodyType::Dynamic, EEnemyArchetype::Grunt},
         {EPresetGroup::Enemies, EPresetAction::SpawnEnemy, LK_ICON_USER_MD, "Jumper", {0.20f, 0.20f}, FColor::White, ETexture::Goblin, EBodyType::Dynamic, EEnemyArchetype::Jumper},
         {EPresetGroup::Enemies, EPresetAction::SpawnEnemy, LK_ICON_CROSSHAIRS, "Ranged", {0.20f, 0.20f}, FColor::White, ETexture::Goblin, EBodyType::Dynamic, EEnemyArchetype::RangedShooter},

         {EPresetGroup::Misc, EPresetAction::SpawnSpawnpoint, LK_ICON_FLAG, "Spawn", {0.10f, 0.10f}, FColor::Convert(RGBA32::SmoothGreen), ETexture::White, EBodyType::Static},
         {EPresetGroup::Misc, EPresetAction::SpawnBlock, LK_ICON_BOMB, "Hazard", {0.25f, 0.25f}, FColor::Convert(RGBA32::Red), ETexture::White, EBodyType::Static},
		 }
    };

	static std::string MakeUniqueName(const std::shared_ptr<CScene>& Scene, std::string_view Prefix)
	{
		for (std::size_t Idx = 1; Idx < 10000; Idx++) {
			std::string Candidate = Format("{}-{}", Prefix, Idx);
			if (!Scene->DoesActorExist(Candidate)) {
				return Candidate;
			}
		}
		return Format("{}-{}", Prefix, std::rand());
	}

	static void ApplyBlockToActorAttr(const FQuickPreset& Preset)
	{
		ActorAttr.Size = Preset.Size;
		ActorAttr.Texture = Preset.Texture;
		EColor Deduced = EColor::White;
		if (FColor::DeduceEnum(Deduced, Preset.Color)) {
			ActorAttr.Color = Deduced;
		}
	}

	/* @todo: Move to CSpawner */
	static void SpawnBlock(const std::shared_ptr<CScene>& Scene, const FQuickPreset& Preset)
	{
		ApplyBlockToActorAttr(Preset);
		const std::string Name = MakeUniqueName(Scene, Preset.Label);
		FBodySpecification BodySpec;
		BodySpec.Type = Preset.BodyType;
		BodySpec.Position = ActorAttr.Position;
		BodySpec.Flags = EBodyFlag_PreSolveEvents;

		LK_INFO_TAG("QuickCreator", "Spawn block: {}", Name);
		if (Preset.BodyType == EBodyType::Static) {
			CSpawner::CreateStaticPolygon(Name, ActorAttr.Position, Preset.Size, BodySpec, Preset.Color, Preset.Texture);
		} else {
			CSpawner::CreatePolygon(Name, BodySpec, Preset.Size, Preset.Color, Preset.Texture);
		}
	}

	/* @todo: Move to CSpawner */
	static void SpawnEnemy(const std::shared_ptr<CScene>& Scene, const FQuickPreset& Preset)
	{
		FEnemySpecification EnemySpec;
		EnemySpec.Archetype = Preset.Archetype;
		EnemySpec.SpawnPoint = ActorAttr.Position;

		FActorSpecification ActorSpec;
		ActorSpec.Name = MakeUniqueName(Scene, Format("Enemy-{}", Enum::ToString(Preset.Archetype)));
		ActorSpec.Texture = Preset.Texture;

		const glm::vec2 EnemySize = GetEnemyArchetype(Preset.Archetype).Size;

		FBodySpecification BodySpec;
		BodySpec.Type = EBodyType::Dynamic;
		BodySpec.Position = ActorAttr.Position;
		BodySpec.Flags = EBodyFlag_PreSolveEvents;
		BodySpec.Shape.emplace<FPolygon>(FPolygon{.Size = EnemySize});

		LK_INFO_TAG("QuickCreator", "Spawn enemy: {} ({})", ActorSpec.Name, Enum::ToString(Preset.Archetype));
		std::shared_ptr<CEnemy> Enemy = Scene->Create<CEnemy>(EnemySpec, ActorSpec, BodySpec);
		Enemy->SetController(std::make_unique<CPatrolController>(1.0f, 1.0f));
		Enemy->AddComponent<FHealthComponent>();
	}

	/* @todo: Move to CSpawner */
	static void SpawnSpawnpoint(const FQuickPreset& Preset)
	{
		LK_UNUSED(Preset);
		LK_INFO_TAG("QuickCreator", "Spawnpoint: ({:.2f}, {:.2f})", ActorAttr.Position.x, ActorAttr.Position.y);
		CSpawner::CreateSpawnpoint("PlayerSpawn", ActorAttr.Position);
	}

	static void OnPresetClicked(const std::shared_ptr<CScene>& Scene, const FQuickPreset& Preset)
	{
		if (!Scene) {
			return;
		}

		switch (Preset.Action) {
			case EPresetAction::SpawnBlock:      SpawnBlock(Scene, Preset); break;
			case EPresetAction::SpawnEnemy:      SpawnEnemy(Scene, Preset); break;
			case EPresetAction::SpawnSpawnpoint: SpawnSpawnpoint(Preset); break;
		}
	}

	void QuickCreator(const std::shared_ptr<CScene>& Scene)
	{
		if (!Scene) {
			return;
		}
		const ImGuiStyle& Style = ImGui::GetStyle();
		const float Avail = ImGui::GetContentRegionAvail().y;

		UI::FScopedColor ChildBg(ImGuiCol_ChildBg, RGBA32::BackgroundDarker);
		UI::FScopedStyle ChildRounding(ImGuiStyleVar_ChildRounding, 8.0f);
		UI::FScopedStyle WindowPadding(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 8.0f));

		ImGui::Dummy(ImVec2(0.0f, 4.0f));

		constexpr ImVec2 ButtonSize = ImVec2(112.0f, 52.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));

		static const std::size_t LargestStringIdx = StringUtils::GetIndexOfLongest(Enum::View<EPresetGroup, std::string_view>());
		static const ImVec2 LargestTextSize = ImGui::CalcTextSize(Enum::ToString<const char*>(Enum::View<EPresetGroup>()[LargestStringIdx]));

		EPresetGroup CurrentGroup = EPresetGroup::COUNT;
		for (std::size_t Idx = 0; Idx < Presets.size(); Idx++) {
			const FQuickPreset& Preset = Presets.at(Idx);
			if (Preset.Group != CurrentGroup) {
				CurrentGroup = Preset.Group;
				UI::FScopedColor LabelColor(ImGuiCol_Text, RGBA32::Text::Darker);
				UI::FScopedFont LabelFont(EFont::Roboto, EFontSize::Regular, EFontModifier::Bold);
				UI::ShiftCursor(6.0f, 12.0f);
				ImGui::TextUnformatted(Enum::ToString<const char*>(CurrentGroup));
				ImGui::SameLine(0.0f, 6.0f);
				UI::ShiftCursorY(-12.0f);
				ImGui::SetCursorPosX(LargestTextSize.x + ImGui::GetFrameHeight());
			} else {
				ImGui::SameLine(0.0f, 4.0f);
				UI::ShiftCursorY(-12.0f);
			}

			UI::FScopedID ScopedID(static_cast<int>(Idx));
			std::array<char, 64> ButtonLabel{};
			std::snprintf(ButtonLabel.data(), ButtonLabel.size(), "%s  %s", Preset.Icon, Preset.Label);
			if (ImGui::Button(ButtonLabel.data(), ButtonSize)) {
				OnPresetClicked(Scene, Preset);
			}
		}

		ImGui::PopStyleVar(2);
	}

}
