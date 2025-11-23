#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/core.h"
#include "core/delegate.h"
#include "core/math/math.h"
#include "renderer/color.h"
#include "renderer/font.h"
#include "renderer/ui/scoped.h"
#include "scene/actor.h"

namespace platformer2d {
	class CScene;
}

namespace platformer2d::UI {

	/* @todo: Use global config */
	constexpr float GAME_MENU_LABEL_COLUMN_WIDTH = 190.0f;
	constexpr float GAME_MENU_LABEL_INDENT_WIDTH = 24.0f;
	constexpr float GAME_MENU_COLUMN_ITEM_WIDTH = 410.0f;

	LK_DECLARE_MULTICAST_DELEGATE(FOnGameMenuOpened, bool);
	extern FOnGameMenuOpened OnGameMenuOpened;

	struct FViewportData
	{
		glm::vec2 MenuBarSize = { 0.0f, 30.0f };
		glm::vec2 LeftSidebarSize = { 340.0f, 0.0f };
		glm::vec2 RightSidebarSize = { 340.0f, 0.0f };
	};
	extern FViewportData ViewportData;
	const FViewportData& GetViewportData();

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
	void CreatorMenuButtons(std::shared_ptr<CScene> Scene);
	void PhysicsBodyMenu(FPhysicsBodyData& Data);

	void TextureModifier();
	bool TextureDropDown(ETexture& Selected);

	/**
	 * @brief Combo dropdown.
	 */
	bool BlendFunction();

	/**
	 * @brief Combo dropdown.
	 */
	bool DepthFunction();

	bool DrawGizmo(uint32_t Operation, CActor& Actor, const glm::mat4& ViewMatrix,
				   const glm::mat4& ProjectionMatrix, const glm::vec3& CameraPos = glm::vec3(0.0f, 0.0f, 0.0f));

	void ColdTextGradient(const char* Text, float Speed = 2.0f);
	void RainbowTextGradient(const char* Text, float Speed = 0.15f);
	void RainbowTextSynced(const char* Text, float WaveLengthPx = 180.0f, float SpeedPxPerSec = 30.0f,
						   float Saturation = 1.0f, float Value = 1.0f);

	void PrepareLeftSidebar();
	void PrepareRightSidebar();

}