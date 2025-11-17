#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "core/core.h"
#include "core/delegate.h"
#include "core/math/math.h"
#include "renderer/font.h"
#include "renderer/ui/scoped.h"
#include "scene/actor.h"

namespace platformer2d {
	class CScene;
}

namespace platformer2d::UI {

	LK_DECLARE_MULTICAST_DELEGATE(FOnGameMenuOpened, bool);
	extern FOnGameMenuOpened OnGameMenuOpened;

	struct FViewportData
	{
		glm::vec2 MenuBarSize = { 0.0f, 30.0f };
		glm::vec2 LeftSidebarSize = { 340.0f, 0.0f };
		glm::vec2 RightSidebarSize = { 340.0f, 0.0f };
	};
	const FViewportData& GetViewportData();

	void OpenGameMenu();
	void CloseGameMenu();
	void ToggleGameMenu();
	bool IsGameMenuOpen();

	void CreatorMenu(std::shared_ptr<CScene> Scene);

	struct FPhysicsBodyData
	{
		EBodyType BodyType = EBodyType::Static;

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
	void PhysicsBodyMenu(FPhysicsBodyData& Data);

	void TextureModifier();
	void TextureDropDown(std::size_t& SelectedIdx);

	/**
	 * @brief Combo dropdown.
	 */
	bool BlendFunction();

	/**
	 * @brief Combo dropdown.
	 */
	bool DepthFunction();

	void DrawGizmo(int Operation, CActor& Actor, const glm::mat4& ViewMatrix,
				   const glm::mat4& ProjectionMatrix, const glm::vec3& CameraPos = glm::vec3(0.0f, 0.0f, 0.0f));

	void ColdTextGradient(const char* Text, float Speed = 2.0f);
	void RainbowTextGradient(const char* Text, float Speed = 0.15f);
	void RainbowTextSynced(const char* Text, float WaveLengthPx = 180.0f, float SpeedPxPerSec = 30.0f,
						   float Saturation = 1.0f, float Value = 1.0f);

	void PrepareLeftSidebar();
	void PrepareRightSidebar();

}