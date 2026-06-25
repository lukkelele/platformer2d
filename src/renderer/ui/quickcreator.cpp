#include "quickcreator.h"

#include "core/log.h"
#include "core/string.h"
#include "game/controller/patrolcontroller.h"
#include "game/enemy.h"
#include "game/spawner.h"
#include "game/weapontype.h"
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
		enum class EPresetGroup : std::uint8_t
		{
			Blocks,
			Enemies,
			Interactable,
			Items,
			Misc,
			COUNT
		};
		LK_ENUM(EPresetGroup);

		struct FQuickPresetVisual
		{
			EPresetGroup Group = EPresetGroup::Blocks;
			const char* Icon = nullptr;
			const char* Label = nullptr;
		};

		struct FBlockSpawn
		{
			glm::vec2 Size = {0.20f, 0.20f};
			glm::vec4 Color = FColor::White;
			ETexture Texture = ETexture::White;
			EBodyType BodyType = EBodyType::Static;
		};

		struct FEnemySpawn
		{
			EEnemyArchetype Archetype = EEnemyArchetype::Grunt;
			ETexture Texture = ETexture::Goblin;
		};

		struct FEnemySpawnerSpawn
		{
			EEnemyArchetype Archetype = EEnemyArchetype::Grunt;
		};

		struct FSpawnpointSpawn
		{
		};

		struct FSensorSpawn
		{
			glm::vec2 Size = {0.20f, 0.20f};
			glm::vec4 Color = FColor::White;
			EInteraction Interaction = EInteraction::None;
		};

		struct FItemSpawn
		{
			glm::vec2 Size = {0.20f, 0.20f};
			glm::vec4 Color = FColor::White;
			ETexture Texture = ETexture::White;
			EWeaponType Weapon = EWeaponType::Rifle;
		};

		struct FTargetDummySpawn
		{
			bool bMoveable = false;
			ETexture Texture = ETexture::Goblin;
		};

		struct FRampSpawn
		{
			glm::vec2 Size = {0.60f, 0.40f};
			glm::vec4 Color = FColor::White;
		};

		using FSpawnPayload = std::variant<FBlockSpawn, FEnemySpawn, FEnemySpawnerSpawn, FSpawnpointSpawn, FSensorSpawn, FItemSpawn, FTargetDummySpawn, FRampSpawn>;

		struct FQuickPreset
		{
			FQuickPresetVisual Visual;
			FSpawnPayload Payload;
		};
	}

	static const std::array<FQuickPreset, 19> Presets = {
		{
         {{EPresetGroup::Blocks, LK_ICON_CUBE, "Small"}, FBlockSpawn{{0.20f, 0.20f}, FColor::Convert(RGBA32::Gray), ETexture::White, EBodyType::Static}},
         {{EPresetGroup::Blocks, LK_ICON_CUBE, "Medium"}, FBlockSpawn{{0.50f, 0.50f}, FColor::Convert(RGBA32::DarkerGray), ETexture::White, EBodyType::Static}},
         {{EPresetGroup::Blocks, LK_ICON_CUBE, "Large"}, FBlockSpawn{{1.0f, 0.30f}, FColor::Convert(RGBA32::Brown), ETexture::White, EBodyType::Static}},
         {{EPresetGroup::Blocks, LK_ICON_CUBE, "Crate"}, FBlockSpawn{{0.30f, 0.30f}, FColor::Convert(RGBA32::Purple), ETexture::White, EBodyType::Dynamic}},
         {{EPresetGroup::Blocks, LK_ICON_TH_LARGE, "Platform"}, FBlockSpawn{{1.20f, 0.15f}, FColor::White, ETexture::Wood, EBodyType::Static}},
         {{EPresetGroup::Blocks, LK_ICON_TH_LARGE, "Ground"}, FBlockSpawn{{1.50f, 0.40f}, FColor::White, ETexture::Bricks, EBodyType::Static}},
         {{EPresetGroup::Blocks, LK_ICON_CUBE, "Wall"}, FBlockSpawn{{0.30f, 1.20f}, FColor::White, ETexture::Metal, EBodyType::Static}},
         {{EPresetGroup::Blocks, LK_ICON_CARET_UP, "Ramp"}, FRampSpawn{{0.60f, 0.40f}, FColor::Convert(RGBA32::Brown)}},

         {{EPresetGroup::Enemies, LK_ICON_USER_SECRET, "Grunt"}, FEnemySpawn{EEnemyArchetype::Grunt, ETexture::Goblin}},
         {{EPresetGroup::Enemies, LK_ICON_USER_MD, "Jumper"}, FEnemySpawn{EEnemyArchetype::Jumper, ETexture::Goblin}},
         {{EPresetGroup::Enemies, LK_ICON_CROSSHAIRS, "Ranged"}, FEnemySpawn{EEnemyArchetype::RangedShooter, ETexture::Goblin}},
         {{EPresetGroup::Enemies, LK_ICON_BANDCAMP, "Spawner"}, FEnemySpawnerSpawn{EEnemyArchetype::Grunt}},
         {{EPresetGroup::Enemies, LK_ICON_BULLSEYE, "Dummy"}, FTargetDummySpawn{false, ETexture::Goblin}},

         {{EPresetGroup::Interactable, LK_ICON_STETHOSCOPE, "Sensor"}, FSensorSpawn{{0.15f, 0.40f}, FColor::LightBlue, EInteraction::Heal}},
         {{EPresetGroup::Interactable, LK_ICON_FLAG_CHECKERED, "Checkpoint"}, FSensorSpawn{{0.15f, 0.40f}, FColor::Convert(RGBA32::SmoothGreen), EInteraction::Checkpoint}},

         {{EPresetGroup::Items, LK_ICON_REBEL, "Rifle"}, FItemSpawn{{0.20f, 0.20f}, FColor::White, ETexture::Rifle, EWeaponType::Rifle}},
         {{EPresetGroup::Items, LK_ICON_GAVEL, "Melee"}, FItemSpawn{{0.20f, 0.20f}, FColor::White, ETexture::White, EWeaponType::Melee}},

         {{EPresetGroup::Misc, LK_ICON_FLAG, "Spawn"}, FSpawnpointSpawn{}},
         {{EPresetGroup::Misc, LK_ICON_BOMB, "Hazard"}, FBlockSpawn{{0.25f, 0.25f}, FColor::Convert(RGBA32::Red), ETexture::White, EBodyType::Static}},
		 }
    };

	static std::string MakeUniqueName(const std::shared_ptr<CScene>& Scene, std::string_view Prefix)
	{
		for (std::size_t Idx = 1; Idx < 10000; Idx++) {
			std::string Candidate = std::format("{}-{}", Prefix, Idx);
			if (!Scene->DoesActorExist(Candidate)) {
				return Candidate;
			}
		}
		return std::format("{}-{}", Prefix, std::rand());
	}

	static void ApplyToActorAttr(const glm::vec2& Size, const glm::vec4& Color, const ETexture Texture)
	{
		ActorAttr.Size = Size;
		ActorAttr.Texture = Texture;
		EColor Deduced = EColor::White;
		if (FColor::DeduceEnum(Deduced, Color)) {
			ActorAttr.Color = Deduced;
		}
	}

	static void SpawnBlock(const FQuickPresetVisual& Visual, const FBlockSpawn& Block, const std::shared_ptr<CScene>& Scene)
	{
		ApplyToActorAttr(Block.Size, Block.Color, Block.Texture);
		const std::string Name = MakeUniqueName(Scene, Visual.Label);
		FBodySpecification BodySpec;
		BodySpec.Type = Block.BodyType;
		BodySpec.Position = ActorAttr.Position;
		BodySpec.Flags = EBodyFlag_PreSolveEvents;

		LK_INFO_TAG("QuickCreator", "Spawn block: {}", Name);
		if (Block.BodyType == EBodyType::Static) {
			CSpawner::CreateStaticPolygon(Name, ActorAttr.Position, Block.Size, BodySpec, Block.Color, Block.Texture);
		} else {
			CSpawner::CreatePolygon(Name, BodySpec, Block.Size, Block.Color, Block.Texture);
		}
	}

	static void SpawnEnemy(const FQuickPresetVisual& Visual, const FEnemySpawn& Enemy, const std::shared_ptr<CScene>& Scene)
	{
		LK_UNUSED(Visual);
		const FEnemyArchetype& Archetype = GetEnemyArchetype(Enemy.Archetype);
		const glm::vec2 EnemySize = Archetype.Size;

		FEnemySpecification EnemySpec;
		EnemySpec.Archetype = Enemy.Archetype;
		EnemySpec.SpawnPoint = ActorAttr.Position;

		FActorSpecification ActorSpec;
		ActorSpec.Name = MakeUniqueName(Scene, std::format("Enemy-{}", Enum::ToString(Enemy.Archetype)));
		ActorSpec.Texture = Enemy.Texture;
		ActorSpec.SpriteScale = Archetype.SpriteScale;

		FBodySpecification BodySpec;
		BodySpec.Type = EBodyType::Dynamic;
		BodySpec.Position = ActorAttr.Position;
		BodySpec.Flags = EBodyFlag_PreSolveEvents | EBodyFlag_ContactEvents;
		BodySpec.Shape.emplace<FPolygon>(FPolygon{.Size = EnemySize});

		LK_INFO_TAG("QuickCreator", "Spawn enemy: {} ({})", ActorSpec.Name, Enum::ToString(Enemy.Archetype));
		std::shared_ptr<CEnemy> SpawnedEnemy = Scene->Create<CEnemy>(EnemySpec, ActorSpec, BodySpec);
		SpawnedEnemy->SetController(std::make_unique<CPatrolController>(1.0f, 1.0f));
		SpawnedEnemy->AddComponent<FHealthComponent>();
	}

	static void SpawnEnemySpawner(const FQuickPresetVisual& Visual, const FEnemySpawnerSpawn& Spawner, const std::shared_ptr<CScene>& Scene)
	{
		LK_UNUSED(Visual);
		const std::string Name = MakeUniqueName(Scene, std::format("EnemySpawner-{}", Enum::ToString(Spawner.Archetype)));
		std::vector<FSpawnWave> Waves = {
			FSpawnWave{.Archetype = Spawner.Archetype}};

		LK_INFO_TAG("QuickCreator", "Spawn enemy spawner: {} ({})", Name, Enum::ToString(Spawner.Archetype));
		CSpawner::CreateEnemySpawner(Name, ActorAttr.Position, std::move(Waves));
	}

	static void SpawnSpawnpoint()
	{
		LK_INFO_TAG("QuickCreator", "Spawnpoint: ({:.2f}, {:.2f})", ActorAttr.Position.x, ActorAttr.Position.y);
		CSpawner::CreateSpawnpoint("PlayerSpawn", ActorAttr.Position);
	}

	static void SpawnSensor(const FQuickPresetVisual& Visual, const FSensorSpawn& Sensor, const std::shared_ptr<CScene>& Scene)
	{
		ApplyToActorAttr(Sensor.Size, Sensor.Color, ETexture::White);
		const std::string Name = MakeUniqueName(Scene, Visual.Label);
		FBodySpecification BodySpec;
		BodySpec.Type = EBodyType::Static;
		BodySpec.Position = ActorAttr.Position;
		BodySpec.Flags = EBodyFlag_PreSolveEvents | EBodyFlag_SensorEvents;
		BodySpec.bSensor = true;

		LK_INFO_TAG("QuickCreator", "Spawn sensor: {}", Name);
		std::shared_ptr<CActor> Actor = CSpawner::CreateStaticPolygon(Name, ActorAttr.Position, Sensor.Size, BodySpec, Sensor.Color, ETexture::White);

		Actor->SetOutlineColor(FColor::Black);
		Actor->SetOutlineThickness(2.0f);

		auto& IC = Actor->AddComponent<FInteractionComponent>();
		IC.Type = Sensor.Interaction;
		switch (IC.Type) {
			case EInteraction::Damage:
			{
				IC.Data = FDamageInteraction{};
				break;
			}
			case EInteraction::Heal:
			{
				IC.Data = FHealInteraction{};
				break;
			}
			case EInteraction::Killzone:
			{
				IC.Data = FKillzoneInteraction{};
				break;
			}
			case EInteraction::Climbable:
			{
				IC.Data = FClimbableInteraction{};
				break;
			}
			case EInteraction::Checkpoint:
			{
				IC.Data = FCheckpointInteraction{};
				break;
			}
			default:
				LK_ERROR_TAG("QuickCreator", "Unhandled interaction: {}", Enum::ToString(IC.Type));
				break;
		}
	}

	static FPickupWeapon MakePickupWeapon(const EWeaponType Weapon)
	{
		FPickupWeapon Pickup{};
		Pickup.Type = Weapon;
		switch (Weapon) {
			case EWeaponType::Rifle:
			{
				FRifleSpecification RifleSpec{};
				RifleSpec.MagazineSize = 30;
				Pickup.Spec = RifleSpec;
				break;
			}
			case EWeaponType::Melee:
				break;
			default:
				LK_ERROR_TAG("QuickCreator", "Unhandled weapon item: {}", Enum::ToString(Weapon));
				break;
		}
		return Pickup;
	}

	static void SpawnItem(const FQuickPresetVisual& Visual, const FItemSpawn& Item, const std::shared_ptr<CScene>& Scene)
	{
		ApplyToActorAttr(Item.Size, Item.Color, Item.Texture);
		const std::string Name = MakeUniqueName(Scene, Visual.Label);
		FBodySpecification BodySpec;
		BodySpec.Type = EBodyType::Static;
		BodySpec.Position = ActorAttr.Position;
		BodySpec.Flags = EBodyFlag_PreSolveEvents | EBodyFlag_SensorEvents;
		BodySpec.bSensor = true;

		LK_INFO_TAG("QuickCreator", "Spawn item: {} ({})", Name, Enum::ToString(Item.Weapon));
		std::shared_ptr<CActor> Actor = CSpawner::CreateStaticPolygon(Name, ActorAttr.Position, Item.Size, BodySpec, Item.Color, Item.Texture);

		Actor->SetOutlineColor(FColor::Magenta);
		Actor->SetOutlineThickness(2.0f);

		FPickupInteraction PI{};
		PI.Kind = EPickupKind::Weapon;
		PI.bExpireWhenPickedUp = false;
		PI.Object = MakePickupWeapon(Item.Weapon);

		auto& IC = Actor->AddComponent<FInteractionComponent>();
		IC.Type = EInteraction::Pickup;
		IC.Data = PI;

		ActorAttr.Reset();
	}

	static void SpawnTargetDummy(const FQuickPresetVisual& Visual, const FTargetDummySpawn& Dummy, const std::shared_ptr<CScene>& Scene)
	{
		LK_UNUSED(Visual);
		const FEnemyArchetype& Archetype = GetEnemyArchetype(EEnemyArchetype::TargetDummy);

		FEnemySpecification EnemySpec;
		EnemySpec.Archetype = EEnemyArchetype::TargetDummy;
		EnemySpec.SpawnPoint = ActorAttr.Position;

		FActorSpecification ActorSpec;
		ActorSpec.Name = MakeUniqueName(Scene, "TargetDummy");
		ActorSpec.Texture = Dummy.Texture;
		ActorSpec.SpriteScale = Archetype.SpriteScale;

		const glm::vec2 DummySize = Archetype.Size;
		FBodySpecification BodySpec;
		BodySpec.Type = Dummy.bMoveable ? EBodyType::Dynamic : EBodyType::Static;
		BodySpec.Position = ActorAttr.Position;
		BodySpec.Flags = EBodyFlag_PreSolveEvents | EBodyFlag_ContactEvents;
		BodySpec.Shape.emplace<FPolygon>(FPolygon{.Size = DummySize});

		LK_INFO_TAG("QuickCreator", "Spawn target dummy: {} (Moveable={})", ActorSpec.Name, Dummy.bMoveable);
		Scene->Create<CEnemy>(EnemySpec, ActorSpec, BodySpec);
	}

	static void SpawnRamp(const FQuickPresetVisual& Visual, const FRampSpawn& Ramp, const std::shared_ptr<CScene>& Scene)
	{
		const std::string Name = MakeUniqueName(Scene, Visual.Label);
		const glm::vec2 P = ActorAttr.Position;
		const float W = Ramp.Size.x;
		const float H = Ramp.Size.y;
		const std::array<glm::vec2, 4> Points = {
			glm::vec2{             P.x,     P.y},
			glm::vec2{P.x + (W * 0.5f),     P.y},
			glm::vec2{         P.x + W,     P.y},
			glm::vec2{         P.x + W, P.y + H}
        };

		LK_INFO_TAG("QuickCreator", "Spawn ramp: {} ({:.2f} x {:.2f})", Name, W, H);
		CSpawner::CreateChain(Name, Points, true, true, Ramp.Color);
	}

	static void OnPresetClicked(const std::shared_ptr<CScene>& Scene, const FQuickPreset& Preset)
	{
		if (!Scene) {
			return;
		}

		/* clang-format off */
		std::visit(TOverload{
			[&](const FBlockSpawn& Block) { SpawnBlock(Preset.Visual, Block, Scene); },
			[&](const FEnemySpawn& Enemy) { SpawnEnemy(Preset.Visual, Enemy, Scene); },
			[&](const FEnemySpawnerSpawn& Spawner) { SpawnEnemySpawner(Preset.Visual, Spawner, Scene); },
			[&](const FSpawnpointSpawn&) { SpawnSpawnpoint(); },
			[&](const FSensorSpawn& Sensor) { SpawnSensor(Preset.Visual, Sensor, Scene); },
			[&](const FItemSpawn& Item) { SpawnItem(Preset.Visual, Item, Scene); },
			[&](const FTargetDummySpawn& Dummy) { SpawnTargetDummy(Preset.Visual, Dummy, Scene); },
			[&](const FRampSpawn& Ramp) { SpawnRamp(Preset.Visual, Ramp, Scene); }},
			Preset.Payload);
		/* clang-format on */
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

		constexpr ImVec2 ButtonSize = ImVec2(132.0f, 52.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));

		static const std::size_t LargestStringIdx = StringUtils::GetIndexOfLongest(Enum::View<EPresetGroup, std::string_view>());
		static const ImVec2 LargestTextSize = ImGui::CalcTextSize(Enum::ToString<const char*>(Enum::View<EPresetGroup>()[LargestStringIdx]));

		constexpr std::size_t ButtonsPerRow = 4;
		const float ButtonStartX = LargestTextSize.x + ImGui::GetFrameHeight() * 2.0f;
		const float RowStride = ButtonSize.y + Style.ItemSpacing.y;

		EPresetGroup CurrentGroup = EPresetGroup::COUNT;
		std::size_t ColInRow = 0;
		std::size_t RowInGroup = 0;
		float GroupStartY = 0.0f;
		for (std::size_t Idx = 0; Idx < Presets.size(); Idx++) {
			const FQuickPreset& Preset = Presets.at(Idx);
			if (Preset.Visual.Group != CurrentGroup) {
				ImGui::Dummy(ImVec2(0, 1));
				CurrentGroup = Preset.Visual.Group;
				ColInRow = 0;
				RowInGroup = 0;
				GroupStartY = ImGui::GetCursorPosY();

				std::size_t GroupCount = 0;
				for (std::size_t I = Idx; (I < Presets.size()) && (Presets.at(I).Visual.Group == CurrentGroup); I++) {
					GroupCount++;
				}
				const std::size_t Rows = (GroupCount + ButtonsPerRow - 1) / ButtonsPerRow;
				const float GroupHeight = (static_cast<float>(Rows) * ButtonSize.y) + (static_cast<float>(Rows - 1) * Style.ItemSpacing.y);

				ImGui::PushStyleColor(ImGuiCol_Text, RGBA32::Text::Darker);
				UI::Font::Push(EFont::Roboto, EFontSize::Regular, EFontModifier::Bold);
				const float LabelHeight = ImGui::GetTextLineHeight();
				float LabelOffsetY = (GroupHeight - LabelHeight) * 0.5f;
				if (LabelOffsetY < 0.0f) {
					LabelOffsetY = 0.0f;
				}
				ImGui::SetCursorPos(ImVec2(16.0f, GroupStartY + LabelOffsetY));
				ImGui::TextUnformatted(Enum::ToString<const char*>(CurrentGroup));
				UI::Font::Pop();
				ImGui::PopStyleColor();

				ImGui::SetCursorPos(ImVec2(ButtonStartX, GroupStartY));
			} else if (ColInRow == 0) {
				RowInGroup++;
				ImGui::SetCursorPos(ImVec2(ButtonStartX, GroupStartY + (static_cast<float>(RowInGroup) * RowStride)));
			} else {
				ImGui::SameLine(0.0f, 4.0f);
			}

			UI::FScopedID ScopedID(static_cast<int>(Idx));
			std::array<char, 64> ButtonLabel{};
			std::snprintf(ButtonLabel.data(), ButtonLabel.size(), "%s  %s", Preset.Visual.Icon, Preset.Visual.Label);
			if (ImGui::Button(ButtonLabel.data(), ButtonSize)) {
				OnPresetClicked(Scene, Preset);
			}

			ColInRow++;
			if (ColInRow >= ButtonsPerRow) {
				ColInRow = 0;
			}
		}

		ImGui::PopStyleVar(2);
	}

}
