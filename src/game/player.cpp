#include "player.h"

#include <imgui/imgui.h>

#include "core/log.h"
#include "core/input/keyboard.h"

namespace platformer2d {

	CPlayer::CPlayer(const FActorSpecification& Specification)
		: CActor(Specification)
	{
	}

	void CPlayer::Tick(const float DeltaTime)
	{
		CActor::Tick(DeltaTime);
		if (CKeyboard::IsKeyDown(EKey::A))
		{
			//TransformComp.Translation.x -= MovementSpeed;
			Body->SetPositionX(TransformComp.Translation.x - MovementSpeed);
		}
		if (CKeyboard::IsKeyDown(EKey::D))
		{
			//TransformComp.Translation.x += MovementSpeed;
			Body->SetPositionX(TransformComp.Translation.x + MovementSpeed);
		}
		if (CKeyboard::IsKeyDown(EKey::W))
		{
			Body->ApplyForce({ 0.0f, 1.0f });
		}
		if (CKeyboard::IsKeyDown(EKey::E))
		{
			Body->ApplyImpulse({ 0.0f, 5.0f });
		}
		if (CKeyboard::IsKeyDown(EKey::R))
		{
			Body->ApplyImpulse({ 0.0f, -5.0f });
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
