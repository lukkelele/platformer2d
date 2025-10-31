#pragma once

#include <string>

#include <glm/glm.hpp>

#include "core/delegate.h"
#include "renderer/camera.h"
#include "scene/actor.h"

namespace platformer2d {

	struct FPlayerData
	{
		uint64_t ID = 0;
		bool bJumping = false;
	};

	class CPlayer : public CActor
	{
	public:
		LK_DECLARE_EVENT(FOnJumped, CPlayer, const FPlayerData&);
		LK_DECLARE_EVENT(FOnLanded, CPlayer, const FPlayerData&);
	public:
		CPlayer(const FActorSpecification& Spec = FActorSpecification(), ETexture InTexture = ETexture::Player);
		CPlayer(const FBodySpecification& BodySpec, ETexture InTexture = ETexture::Player);
		CPlayer(CPlayer&&) = default;
		CPlayer(const CPlayer&) = default;
		~CPlayer() = default;

		virtual void Tick(float DeltaTime) override;
		void Jump();

		const FPlayerData& GetData() const { return Data; }

		inline float GetJumpImpulse() const { return JumpImpulse; }
		void SetJumpImpulse(float Impulse);
		inline float GetDirectionForce() const { return DirForce; }
		void SetDirectionForce(float Force);

		inline CCamera& GetCamera() { return *Camera; }
		inline const CCamera& GetCamera() const { return *Camera; }

	private:
		void CheckJumpState();
		void OnWindowResize(uint16_t Width, uint16_t Height);

	public:
		FOnJumped OnJumped;
		FOnLanded OnLanded;
	private:
		FPlayerData Data{};
		std::unique_ptr<CCamera> Camera = nullptr;

		float JumpImpulse = 0.530f;
		float DirForce = 5.630f;
	};

}