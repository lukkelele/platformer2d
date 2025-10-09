#include "player.h"

#include <imgui/imgui.h>

#include "core/log.h"
#include "input/keyboard.h"

namespace platformer2d {

	CPlayer::CPlayer(std::string_view InName)
		: Name(InName)
	{
	}

	void CPlayer::Tick(const float DeltaTime)
	{
		//if (ImGui::IsKeyDown(ImGuiKey::ImGuiKey_A))
		if (CKeyboard::IsKeyDown(EKey::A))
		{
			Pos.x -= MovementSpeed;
		}
		//if (ImGui::IsKeyDown(ImGuiKey::ImGuiKey_D))
		if (CKeyboard::IsKeyDown(EKey::D))
		{
			Pos.x += MovementSpeed;
		}
	}

	void CPlayer::SetPosition(const float X, const float Y)
	{
		Pos.x = X;
		Pos.y = Y;
		LK_DEBUG("Pos=({}, {})", Pos.x, Pos.y);
	}

	void CPlayer::SetPosition(const glm::vec2& NewPos)
	{
		Pos = NewPos;
		LK_DEBUG("Pos=({}, {})", Pos.x, Pos.y);
	}

	float CPlayer::GetMovementSpeed() const
	{
		return (MovementSpeed * MovementSpeedFactor);
	}

	void CPlayer::SetMovementSpeed(const float NewSpeed)
	{
		MovementSpeed = (NewSpeed / 10000.0f);
		LK_TRACE("MovementSpeed={} (NewSpeed={})", MovementSpeed, NewSpeed);
	}

}
