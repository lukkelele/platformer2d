#include "player.h"

#include <imgui/imgui.h>

#include "core/log.h"
#include "core/input/keyboard.h"
#include "core/window.h"

namespace platformer2d {

	CPlayer::CPlayer(const FActorSpecification& Spec, const ETexture InTexture)
		: CActor(Spec, InTexture)
	{
		Camera = std::make_unique<CCamera>(SCREEN_WIDTH, SCREEN_HEIGHT);
		CWindow::OnResized.Add(this, &CPlayer::OnWindowResized);
		CMouse::OnScrolled.Add(this, &CPlayer::OnMouseScrolled);
	}

	CPlayer::CPlayer(const FBodySpecification& BodySpec, const ETexture InTexture)
		: CActor(BodySpec, InTexture)
	{
		Camera = std::make_unique<CCamera>(SCREEN_WIDTH, SCREEN_HEIGHT);
		CWindow::OnResized.Add(this, &CPlayer::OnWindowResized);
		CMouse::OnScrolled.Add(this, &CPlayer::OnMouseScrolled);
	}

	void CPlayer::Tick(const float DeltaTime)
	{
		CActor::Tick(DeltaTime);

		CheckJumpState();

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

		if (bCameraLock)
		{
			Camera->Target(Body->GetPosition(), DeltaTime);
		}
		Camera->Update();
	}

	void CPlayer::Jump()
	{
		if (!Data.bJumping)
		{
			Data.bJumping = true;
			Body->ApplyImpulse({ 0.0f, JumpImpulse });
			OnJumped.Broadcast(Data);
		}
	}

	void CPlayer::SetJumpImpulse(const float Impulse)
	{
		JumpImpulse = Impulse;
	}

	void CPlayer::SetDirectionForce(const float Force)
	{
		DirForce = Force;
	}

	void CPlayer::SetCameraLock(bool Locked)
	{
		bCameraLock = true;
	}

	void CPlayer::CheckJumpState()
	{
		static constexpr int MAX_CONTACTS = 4;
		const b2BodyId BodyID = Body->GetID();
		const int Capacity = std::min(b2Body_GetContactCapacity(BodyID), MAX_CONTACTS);

		bool bCanJump = false;
		b2ContactData ContactData[MAX_CONTACTS];
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

		if (bCanJump && Data.bJumping)
		{
			Data.bJumping = false;
			OnLanded.Broadcast(Data);
		}
	}

	/* @fixme This might be redundant as the camera does set the viewport size on every scene start */
	void CPlayer::OnWindowResized(const uint16_t Width, const uint16_t Height)
	{
		if (Camera)
		{
			Camera->SetViewportSize(Width, Height);
		}
	}

	void CPlayer::OnMouseScrolled(const EMouseScrollDirection Direction)
	{
		LK_TRACE_TAG("Player", "Mouse scroll: {}", Enum::ToString(Direction));
		if (CKeyboard::IsKeyDown(EKey::LeftControl) || CKeyboard::IsKeyDown(EKey::RightControl))
		{
			if (Camera)
			{
				const float ZoomDiff = (Direction == EMouseScrollDirection::Up)
					? -CCamera::ZOOM_DIFF
					: CCamera::ZOOM_DIFF;
				Camera->SetZoom(Camera->GetZoom() + ZoomDiff);
			}
		}
	}

}
