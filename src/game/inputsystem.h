#pragma once

#include "core/core.h"
#include "core/delegate.h"
#include "core/input/keycodes.h"
#include "core/input/mousecodes.h"
#include "core/input/gamepadcodes.h"
#include "gamesystem.h"
#include "actioncodes.h"

namespace platformer2d {

	class CInputSystem : public IGameSystem
	{
	public:
		LK_DECLARE_EVENT(FOnActionPressed, CInputSystem, EAction);
		LK_DECLARE_EVENT(FOnActionReleased, CInputSystem, EAction);
		LK_DECLARE_EVENT(FOnAxisChanged, CInputSystem, EAxis, float);
		LK_DECLARE_EVENT(FOnPadConnected, CInputSystem, std::int32_t);
		LK_DECLARE_EVENT(FOnPadDisconnected, CInputSystem, std::int32_t);

	public:
		CInputSystem() = default;
		CInputSystem(CInputSystem&&) = delete;
		CInputSystem(const CInputSystem&) = delete;
		~CInputSystem() override = default;

		CInputSystem& operator=(CInputSystem&&) = delete;
		CInputSystem& operator=(const CInputSystem&) = delete;

		void Initialize(CGameInstance& Owner) override;
		void Shutdown() override;
		void Tick() override;

		void BindAction(EAction Action, EKey Key);
		void BindAction(EAction Action, EMouseButton Button);
		void BindAction(EAction Action, EPadButton Button);

		void BindAxis(EAxis Axis, EKey Key, float Scale);
		void BindAxis(EAxis Axis, EPadAxis PadAxis, float Scale = 1.0f, float DeadZone = AXIS_DEAD_ZONE_DEFAULT);

		void ClearBindings(EAction Action);
		void ClearBindings(EAxis Axis);
		void ClearAllBindings();

		[[nodiscard]] bool IsActionDown(EAction Action) const;
		[[nodiscard]] bool IsActionPressed(EAction Action) const;
		[[nodiscard]] bool IsActionReleased(EAction Action) const;
		[[nodiscard]] EActionState GetActionState(EAction Action) const;

		[[nodiscard]] float GetAxis(EAxis Axis) const;

		[[nodiscard]] bool IsPadConnected(std::int32_t PadId) const;
		[[nodiscard]] std::size_t GetConnectedPadCount() const { return ConnectedPadCount; }

		[[nodiscard]] bool IsPadButtonDown(std::int32_t PadId, EPadButton Button) const;
		[[nodiscard]] float GetPadAxisRaw(std::int32_t PadId, EPadAxis Axis) const;

	private:
		void RegisterDefaultBindings();
		void PollGamepads();
		void RefreshActionStates();
		void RefreshAxisValues();

		[[nodiscard]] float ResolveAxisValue(EAxis Axis) const;
		[[nodiscard]] bool ResolveActionDown(EAction Action) const;

		static float ApplyDeadZone(float Value, float DeadZone);
		static float TriggerToUnit(float RawValue);
		static float CombineContribution(float Current, float Contribution);

	public:
		static inline FOnActionPressed OnActionPressed;
		static inline FOnActionReleased OnActionReleased;
		static inline FOnAxisChanged OnAxisChanged;
		static inline FOnPadConnected OnPadConnected;
		static inline FOnPadDisconnected OnPadDisconnected;

		static constexpr float AXIS_DEAD_ZONE_DEFAULT = 0.18f;
		static constexpr float TRIGGER_PRESS_THRESHOLD = 0.50f;
		static constexpr float AXIS_CHANGE_EPSILON = 1e-4f;
		static constexpr std::int32_t MAX_PADS = 16; /* GLFW_JOYSTICK_LAST + 1 */
		static constexpr std::size_t PAD_BUTTON_COUNT = 15;
		static constexpr std::size_t PAD_AXIS_COUNT = 6;

	private:
		struct FKeyAxisBinding
		{
			EKey Key{};
			float Scale = 1.0f;
		};

		struct FPadAxisBinding
		{
			EPadAxis Axis{};
			float Scale = 1.0f;
			float DeadZone = AXIS_DEAD_ZONE_DEFAULT;
		};

		struct FActionBindings
		{
			std::vector<EKey> Keys;
			std::vector<EMouseButton> MouseButtons;
			std::vector<EPadButton> PadButtons;
		};

		struct FAxisBindings
		{
			std::vector<FKeyAxisBinding> Keys;
			std::vector<FPadAxisBinding> PadAxes;
		};

		struct FActionRuntime
		{
			bool bDown = false;
			bool bDownLastFrame = false;
		};

		struct FAxisRuntime
		{
			float Value = 0.0f;
			float OldValue = 0.0f;
		};

		struct FPadState
		{
			bool bConnected = false;
			std::array<std::uint8_t, PAD_BUTTON_COUNT> Buttons{};
			std::array<float, PAD_AXIS_COUNT> Axes{};
		};

		std::array<FActionBindings, std::to_underlying(EAction::COUNT)> ActionBindings{};
		std::array<FAxisBindings, std::to_underlying(EAxis::COUNT)> AxisBindings{};
		std::array<FActionRuntime, std::to_underlying(EAction::COUNT)> Actions{};
		std::array<FAxisRuntime, std::to_underlying(EAxis::COUNT)> Axes{};
		std::array<FPadState, MAX_PADS> Pads{};
		std::size_t ConnectedPadCount = 0;
	};

}

