#include "player.h"

#include <imgui/imgui.h>

#include "core/log.h"
#include "core/input/keyboard.h"

namespace platformer2d {

	CPlayer::CPlayer(std::string_view InName)
		: Name(InName)
	{
	}

	void CPlayer::Tick(const float DeltaTime)
	{
		if (CKeyboard::IsKeyDown(EKey::A))
		{
			TransformComp.Translation.x -= MovementSpeed;
		}
		if (CKeyboard::IsKeyDown(EKey::D))
		{
			TransformComp.Translation.x += MovementSpeed;
		}

		/* Main menu. */
		if (CKeyboard::IsKeyDown(EKey::Escape) && !CKeyboard::IsKeyHeld(EKey::Escape))
		{
			const FKeyData& KeyData = CKeyboard::GetKeyData(EKey::Escape);
			LK_DEBUG("Key: Escape (Repeat={} State={} OldState={})", 
					 KeyData.RepeatCount, KeyData.State, KeyData.OldState);
		}
	}

	void CPlayer::Jump()
	{
		OnJumped.Broadcast(Data);
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

	void CPlayer::SetMovementSpeedFactor(const float SpeedFactor)
	{
		MovementSpeedFactor = SpeedFactor;
	}

}
