#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/core.h"
#include "core/delegate.h"
#include "core/math/math.h"
#include "game/enemy.h"
#include "renderer/color.h"
#include "renderer/font.h"
#include "renderer/texture.h"
#include "renderer/ui/ui_core.h"
#include "renderer/ui/scoped.h"
#include "scene/actor.h"
#include "combo.h"
#include "pausemenu.h"

namespace platformer2d {
	class CPlayer;
	class CScene;
	class CRifle;
}

namespace platformer2d::UI {

	/* @todo: Use global config */
	constexpr float GAME_MENU_LABEL_COLUMN_WIDTH = 190.0f;
	constexpr float GAME_MENU_LABEL_INDENT_WIDTH = 24.0f;
	constexpr float GAME_MENU_COLUMN_ITEM_WIDTH = 410.0f;

	enum class EWidgetPlacement
	{
		Center,
		TopLeft,
		TopRight,
		BottomLeft,
		BottomRight,
	};

	LK_DECLARE_MULTICAST_DELEGATE(FOnPauseMenuOpened, bool);
	extern FOnPauseMenuOpened OnPauseMenuOpened;

	inline bool InTable()
	{
		return ImGui::GetCurrentTable() != nullptr;
	}

	namespace Table {
		inline void Label(std::string_view Str, const float IndentX = 0.0f)
		{
			ImGui::TableSetColumnIndex(0);
			UI::ShiftCursorX(17.0f + IndentX);
			ImGui::AlignTextToFramePadding();
			ImGui::Text(Str.data());
		}

		inline void NextColumn()
		{
			ImGui::TableSetColumnIndex(1);
			UI::ShiftCursorX(7);
			ImGui::AlignTextToFramePadding();
		};

		inline void NextRow()
		{
			ImGui::TableNextRow();
		};
	}

	inline bool Checkbox(std::string_view Str, bool& Value, const float IndentX = 6.0f)
	{
		bool Active = false;
		std::array<char, 64> LabelBuf = {0};
		std::snprintf(LabelBuf.data(), LabelBuf.size(), "##%s", Str.data());

		if (InTable()) {
			Table::Label(Str);
			Table::NextColumn();
			ShiftCursorX(IndentX);
			if (ImGui::Checkbox(LabelBuf.data(), &Value)) {
				Active = true;
			}
		} else {
			ImGui::Text(Str.data());
			ImGui::SameLine(0.0f, IndentX);
			if (ImGui::Checkbox(LabelBuf.data(), &Value)) {
				Active = true;
			}
		}

		return Active;
	}

	bool BeginPropertyGrid(std::size_t LabelColumnWidth = 180.0f);
	void EndPropertyGrid();

	struct FPhysicsBodyData
	{
		EBodyType BodyType = EBodyType::Static;
		glm::vec3 Position = {0.0f, 0.0f, 0.0f};
		float Friction = 0.60f;
		float Density = 1.0f;
		glm::vec2 LinearVelocity = {0.0f, 0.0f};
		float AngularVelocity = 0.0f;
		float GravityScale = 1.0f;
		float LinearDamping = 0.0f;
		float AngularDamping = 0.0f;
		float DirForce = 5.630f;
		float JumpImpulse = 0.530f;

		struct
		{
			bool bPreSolveEvents = true;
			bool bContactEvents = false;
			bool bSensorEvents = false;
			bool bBullet = false;
		} BodyFlag;

		struct
		{
			bool X = false;
			bool Y = false;
			bool Z = false;
			bool All = false;
		} MotionLock;
	};
	extern FPhysicsBodyData PhysicsBodyData;
	void Aggregate(const FPhysicsBodyData& Data, FBodySpecification& BodySpec);

	void LevelLauncher();
	void OpenPauseMenu(EPauseMenuView View = EPauseMenuView::Default);
	void ClosePauseMenu(EPauseMenuView View = EPauseMenuView::Default);
	void TogglePauseMenu();
	bool IsPauseMenuOpen();
	bool MainMenuButton(const ImVec2& Size);

	bool ColorDropdown(EColor& Selected);

	struct FActorAttributes
	{
		glm::vec2 Position = {0.0f, 0.0f};
		glm::vec2 Size = {0.20f, 0.20f};
		ETexture Texture = ETexture::White;
		EColor Color = EColor::White;
		std::array<char, 128> NameBuf = {0};
	};
	extern FActorAttributes ActorAttr;

	bool ActorAttributes(FActorAttributes& Attr);

	void CreatorMenu(std::shared_ptr<CScene> Scene);
	void ActorCreateButtons(std::shared_ptr<CScene> Scene);
	void PhysicsBodyMenu(FPhysicsBodyData& Data);

	struct FChainCreatorState
	{
		std::vector<glm::vec2> Points = {
			{ -1.0f, 0.0f},
			{-0.50f, 0.0f},
			{ 0.50f, 0.0f},
			{  1.0f, 0.0f},
		};
		glm::vec2 PreviewOrigin = {0.0f, 0.0f}; /* World-space position used for new chains and the preview. */
		bool bLoop = false;
		bool bBlockBothSides = false;
		EColor Color = EColor::White;
		std::array<char, 64> NameBuf = {0};
		LUUID EditTarget = LUUID::Null;
		bool bHasEditTarget = false;
		bool bPreviewVisible = true;

		struct
		{
			bool bLastNodeState = false;
			bool bPreviewVisible = true;
		} Cache;

		void ResetPoints()
		{
			Points = {
				{ -1.0f, 0.0f},
				{-0.50f, 0.0f},
				{ 0.50f, 0.0f},
				{  1.0f, 0.0f},
			};
		}

		void OnDeselect()
		{
			ResetPoints();
			EditTarget = LUUID::Null;
			bHasEditTarget = false;
			PreviewOrigin = {0.0f, 0.0f};
		}
	};
	extern FChainCreatorState ChainCreator;
	void ChainCreatorWidget(std::shared_ptr<CScene> Scene);
	void RenderChainPreview(const std::shared_ptr<CScene>& Scene);

	bool DrawGizmo(uint32_t Operation, CActor& Actor, const glm::mat4& ViewMatrix,
		const glm::mat4& ProjectionMatrix, const glm::vec3& CameraPos = glm::vec3(0.0f, 0.0f, 0.0f));

	void PlayerData(std::shared_ptr<CPlayer> Player);
	void RifleData(std::shared_ptr<CRifle> Rifle);
	void Statistics(EWidgetPlacement Placement = EWidgetPlacement::TopLeft);
	void PlayerHud(std::shared_ptr<CPlayer> Player);
	void EnemiesInfo(std::shared_ptr<CScene> Scene);

	void ColdTextGradient(const char* Text, float Speed = 2.0f);
	void RainbowTextGradient(const char* Text, float Speed = 0.15f);
	void RainbowTextSynced(const char* Text, float WaveLengthPx = 180.0f, float SpeedPxPerSec = 30.0f,
		float Saturation = 1.0f, float Value = 1.0f);

	void PrepareLeftSidebar();
	void PrepareRightSidebar();
	void PrepareTopBar();
	void PrepareMenuBar();
	void PrepareBottomBar();

	namespace Array {
		inline constexpr std::array<EDirection, std::to_underlying(EDirection::COUNT)> Direction = {
			EDirection::Up,
			EDirection::Down,
			EDirection::Left,
			EDirection::Right};

		inline constexpr std::array<const char*, std::to_underlying(ETexture::COUNT)> TextureNames = {
			Enum::ToString(ETexture::White),
			Enum::ToString(ETexture::Background),
			Enum::ToString(ETexture::Player),
			Enum::ToString(ETexture::Metal),
			Enum::ToString(ETexture::Bricks),
			Enum::ToString(ETexture::Wood),
			Enum::ToString(ETexture::Swoosh),
			Enum::ToString(ETexture::Cloud),
			Enum::ToString(ETexture::Rifle),
			Enum::ToString(ETexture::Enemy1),
			Enum::ToString(ETexture::Enemy2),
		};
		static_assert(TextureNames.at(std::to_underlying(ETexture::COUNT) - 1) != nullptr);

		inline constexpr std::array<EPickupKind, std::to_underlying(EPickupKind::COUNT)> PickupKind = {
			EPickupKind::Item,
			EPickupKind::Weapon,
		};

		inline constexpr std::array<EEnemyState, std::to_underlying(EEnemyState::COUNT)> EnemyStates = {
			EEnemyState::Idle,
			EEnemyState::Patrolling,
		};
		inline constexpr std::array<const char*, std::to_underlying(EEnemyState::COUNT)> EnemyStateStrings = {
			Enum::ToString(EEnemyState::Idle),
			Enum::ToString(EEnemyState::Patrolling),
		};
		static_assert(EnemyStateStrings.at(std::to_underlying(EEnemyState::COUNT) - 1) != nullptr);
	}

}
