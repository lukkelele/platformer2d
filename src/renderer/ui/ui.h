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

	LK_DECLARE_MULTICAST_DELEGATE(FOnGameMenuOpened, bool);
	extern FOnGameMenuOpened OnGameMenuOpened;

	FORCEINLINE bool InTable() { return ImGui::GetCurrentTable() != nullptr; }

	namespace Table {
		FORCEINLINE void Label(std::string_view Str, const float IndentX = 0.0f)
		{
			ImGui::TableSetColumnIndex(0);
			UI::ShiftCursor(17.0f + IndentX, 0.0f);
			ImGui::Text(Str.data());
		}

		FORCEINLINE void NextColumn()
		{
			ImGui::TableSetColumnIndex(1);
		};
	}

	inline bool Checkbox(std::string_view Str, bool& Value, const float IndentX = 0.0f)
	{
		bool Active = false;
		char LabelBuf[64] = { 0 };
		std::snprintf(LabelBuf, sizeof(LabelBuf), "##%s", Str.data());

		if (InTable()) {
			Table::Label(Str);
			Table::NextColumn();
			UI::ShiftCursor(7.0f + IndentX, 0.0f);
			if (ImGui::Checkbox(LabelBuf, &Value)) {
				Active = true;
			}
		} else {
			ImGui::Text(Str.data());
			ImGui::SameLine(0.0f, IndentX);
			if (ImGui::Checkbox(LabelBuf, &Value)) {
				Active = true;
			}
		}

		return Active;
	}

	void BeginPropertyGrid(std::size_t LabelColumnWidth = 180.0f);
	void EndPropertyGrid();

	struct FPhysicsBodyData
	{
		EBodyType BodyType = EBodyType::Static;
		glm::vec3 Position = { 0.0f, 0.0f, 0.0f };
		float Friction = 0.60f;
		float Density = 1.0f;
		glm::vec2 LinearVelocity = { 0.0f, 0.0f };
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

	void OpenGameMenu();
	void CloseGameMenu();
	void ToggleGameMenu();
	bool IsGameMenuOpen();

	bool ColorDropdown(EColor& Selected);

	struct FActorAttributes
	{
		glm::vec2 Position = { 0.0f, 0.0f };
		glm::vec2 Size = { 0.20f, 0.20f };
		ETexture Texture = ETexture::White;
		EColor Color = EColor::White;
		std::array<char, 128> NameBuf = { 0 };
	};
	extern FActorAttributes ActorAttr;

	bool ActorAttributes(FActorAttributes& Attr);

	void CreatorMenu(std::shared_ptr<CScene> Scene);
	void ActorCreateButtons(std::shared_ptr<CScene> Scene);
	void PhysicsBodyMenu(FPhysicsBodyData& Data);

	bool DrawGizmo(uint32_t Operation, CActor& Actor, const glm::mat4& ViewMatrix,
				   const glm::mat4& ProjectionMatrix, const glm::vec3& CameraPos = glm::vec3(0.0f, 0.0f, 0.0f));

	void PlayerData(std::shared_ptr<CPlayer> Player);
	void RifleData(std::shared_ptr<CRifle> Rifle);
	void Statistics(EWidgetPlacement Placement = EWidgetPlacement::TopLeft);
	void PlayerHud(std::shared_ptr<CPlayer> Player);

	void ColdTextGradient(const char* Text, float Speed = 2.0f);
	void RainbowTextGradient(const char* Text, float Speed = 0.15f);
	void RainbowTextSynced(const char* Text, float WaveLengthPx = 180.0f, float SpeedPxPerSec = 30.0f,
						   float Saturation = 1.0f, float Value = 1.0f);

	void PrepareLeftSidebar();
	void PrepareRightSidebar();
	void PrepareTopBar();

	namespace Array {
		static constexpr std::array<const char*, std::to_underlying(ETexture::COUNT)> TextureNames = {
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

		static constexpr std::array<EPickupKind, std::to_underlying(EPickupKind::COUNT)> PickupKind = {
			EPickupKind::Item,
			EPickupKind::Weapon,
		};

		static constexpr std::array<EEnemyState, std::to_underlying(EEnemyState::COUNT)> EnemyStates = {
			EEnemyState::Idle,
			EEnemyState::Patrolling,
		};
		static constexpr std::array<const char*, std::to_underlying(EEnemyState::COUNT)> EnemyStateStrings = {
			Enum::ToString(EEnemyState::Idle),
			Enum::ToString(EEnemyState::Patrolling),
		};
		static_assert(EnemyStateStrings.at(std::to_underlying(EEnemyState::COUNT) - 1) != nullptr);
	}

}