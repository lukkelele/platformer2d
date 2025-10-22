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

		const b2BodyId BodyID = Body->GetID();
		int Capacity = b2Body_GetContactCapacity(BodyID);
		Capacity = b2MinInt(Capacity, 4);

		bool bCanJump = false;
		b2ContactData ContactData[4];
		const int Count = b2Body_GetContactData(BodyID, ContactData, Capacity);
		for (int Idx = 0; Idx < Count; Idx++)
		{
			b2BodyId BodyA = b2Shape_GetBody(ContactData[Idx].shapeIdA);
			const float Sign = (B2_ID_EQUALS(BodyA, BodyID)) ? -1.0f : 1.0f;
			if (Sign * ContactData[Idx].manifold.normal.y > 0.90f)
			{
				bCanJump = true;
				break;
			}
		}

		if (CKeyboard::IsKeyDown(EKey::A))
		{
			Body->ApplyForce({ -DirForce, 0.0f });
		}
		if (CKeyboard::IsKeyDown(EKey::D))
		{
			Body->ApplyForce({ DirForce, 0.0f });
		}
		if (CKeyboard::IsKeyDown(EKey::Space))
		{
			Jump();
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
		Body->ApplyImpulse({ 0.0f, JumpImpulse });
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

	void CPlayer::SetJumpImpulse(const float Impulse)
	{
		JumpImpulse = Impulse;
	}

}
