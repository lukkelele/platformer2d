#include "inputsystem.h"

#include <GLFW/glfw3.h>

#include "core/log.h"
#include "core/profiler.h"
#include "core/input/keyboard.h"
#include "core/input/mouse.h"

namespace platformer2d {

	void CInputSystem::Initialize(CGameInstance& Owner)
	{
		LK_DEBUG_TAG("InputSystem", "Initialize");
		OwnerRef = &Owner;

		for (FActionRuntime& Action : Actions) {
			Action = {};
		}
		for (FAxisRuntime& Axis : Axes) {
			Axis = {};
		}
		for (FPadState& Pad : Pads) {
			Pad = {};
		}
		ConnectedPadCount = 0;

		RegisterDefaultBindings();
	}

	void CInputSystem::Shutdown()
	{
		LK_DEBUG_TAG("InputSystem", "Shutdown");
		ClearAllBindings();

		for (FActionRuntime& Action : Actions) {
			Action = {};
		}
		for (FAxisRuntime& Axis : Axes) {
			Axis = {};
		}
		for (FPadState& Pad : Pads) {
			Pad = {};
		}

		ConnectedPadCount = 0;
	}

	void CInputSystem::Tick()
	{
		LK_PROFILE_FUNC();

		PollGamepads();
		RefreshActionStates();
		RefreshAxisValues();
	}

	void CInputSystem::BindAction(const EAction Action, const EKey Key)
	{
		LK_ASSERT((Action != EAction::None) && (Action != EAction::COUNT));
		ActionBindings[std::to_underlying(Action)].Keys.push_back(Key);
	}

	void CInputSystem::BindAction(const EAction Action, const EMouseButton Button)
	{
		LK_ASSERT((Action != EAction::None) && (Action != EAction::COUNT));
		ActionBindings[std::to_underlying(Action)].MouseButtons.push_back(Button);
	}

	void CInputSystem::BindAction(const EAction Action, const EPadButton Button)
	{
		LK_ASSERT((Action != EAction::None) && (Action != EAction::COUNT));
		ActionBindings[std::to_underlying(Action)].PadButtons.push_back(Button);
	}

	void CInputSystem::BindAxis(const EAxis Axis, const EKey Key, const float Scale)
	{
		LK_ASSERT((Axis != EAxis::None) && (Axis != EAxis::COUNT));
		AxisBindings[std::to_underlying(Axis)].Keys.push_back({.Key = Key, .Scale = Scale});
	}

	void CInputSystem::BindAxis(const EAxis Axis, const EPadAxis PadAxis, const float Scale, const float DeadZone)
	{
		LK_ASSERT((Axis != EAxis::None) && (Axis != EAxis::COUNT));
		AxisBindings[std::to_underlying(Axis)].PadAxes.push_back({.Axis = PadAxis, .Scale = Scale, .DeadZone = DeadZone});
	}

	void CInputSystem::ClearBindings(const EAction Action)
	{
		LK_ASSERT((Action != EAction::None) && (Action != EAction::COUNT));
		ActionBindings[std::to_underlying(Action)] = {};
	}

	void CInputSystem::ClearBindings(const EAxis Axis)
	{
		LK_ASSERT((Axis != EAxis::None) && (Axis != EAxis::COUNT));
		AxisBindings[std::to_underlying(Axis)] = {};
	}

	void CInputSystem::ClearAllBindings()
	{
		LK_TRACE_TAG("InputSystem", "Clearing all bindings");
		for (FActionBindings& Binding : ActionBindings) {
			Binding = {};
		}
		for (FAxisBindings& Binding : AxisBindings) {
			Binding = {};
		}
	}

	bool CInputSystem::IsActionDown(const EAction Action) const
	{
		LK_ASSERT((Action != EAction::None) && (Action != EAction::COUNT));
		return Actions[std::to_underlying(Action)].bDown;
	}

	bool CInputSystem::IsActionPressed(const EAction Action) const
	{
		LK_ASSERT((Action != EAction::None) && (Action != EAction::COUNT));
		const FActionRuntime& Runtime = Actions[std::to_underlying(Action)];
		return (Runtime.bDown && !Runtime.bDownLastFrame);
	}

	bool CInputSystem::IsActionReleased(const EAction Action) const
	{
		LK_ASSERT((Action != EAction::None) && (Action != EAction::COUNT));
		const FActionRuntime& Runtime = Actions[std::to_underlying(Action)];
		return (!Runtime.bDown && Runtime.bDownLastFrame);
	}

	EActionState CInputSystem::GetActionState(const EAction Action) const
	{
		LK_ASSERT((Action != EAction::None) && (Action != EAction::COUNT));
		const FActionRuntime& Runtime = Actions[std::to_underlying(Action)];
		if (Runtime.bDown && !Runtime.bDownLastFrame) {
			return EActionState::Pressed;
		}
		if (Runtime.bDown && Runtime.bDownLastFrame) {
			return EActionState::Held;
		}
		if (!Runtime.bDown && Runtime.bDownLastFrame) {
			return EActionState::Released;
		}
		return EActionState::None;
	}

	float CInputSystem::GetAxis(const EAxis Axis) const
	{
		LK_ASSERT((Axis != EAxis::None) && (Axis != EAxis::COUNT));
		return Axes[std::to_underlying(Axis)].Value;
	}

	bool CInputSystem::IsPadConnected(const std::int32_t PadId) const
	{
		if ((PadId < 0) || (PadId >= MAX_PADS)) {
			return false;
		}
		return Pads[PadId].bConnected;
	}

	bool CInputSystem::IsPadButtonDown(const std::int32_t PadId, const EPadButton Button) const
	{
		if ((PadId < 0) || (PadId >= MAX_PADS)) {
			return false;
		}
		const FPadState& Pad = Pads.at(PadId);
		if (!Pad.bConnected) {
			return false;
		}

		const std::size_t Idx = std::to_underlying(Button);
		return (Idx < PAD_BUTTON_COUNT) ? (Pad.Buttons[Idx] == GLFW_PRESS) : false;
	}

	float CInputSystem::GetPadAxisRaw(const std::int32_t PadId, const EPadAxis Axis) const
	{
		if ((PadId < 0) || (PadId >= MAX_PADS)) {
			return 0.0f;
		}
		const FPadState& Pad = Pads.at(PadId);
		if (!Pad.bConnected) {
			return 0.0f;
		}

		const std::size_t Idx = std::to_underlying(Axis);
		return (Idx < PAD_AXIS_COUNT) ? Pad.Axes[Idx] : 0.0f;
	}

	void CInputSystem::RegisterDefaultBindings()
	{
		LK_DEBUG_TAG("InputSystem", "Register default bindings");
		/* Digital actions. */
		BindAction(EAction::Jump, EKey::Space);
		BindAction(EAction::Jump, EPadButton::A);
		BindAction(EAction::Fire, EMouseButton::Left);
		BindAction(EAction::Fire, EPadButton::X);
		BindAction(EAction::Interact, EKey::E);
		BindAction(EAction::Interact, EPadButton::Y);
		BindAction(EAction::Pause, EKey::Escape);
		BindAction(EAction::Pause, EPadButton::Start);

		BindAction(EAction::MoveLeft, EKey::A);
		BindAction(EAction::MoveRight, EKey::D);
		BindAction(EAction::MoveUp, EKey::W);
		BindAction(EAction::MoveDown, EKey::S);
		BindAction(EAction::MoveLeft, EPadButton::DPadLeft);
		BindAction(EAction::MoveRight, EPadButton::DPadRight);
		BindAction(EAction::MoveUp, EPadButton::DPadUp);
		BindAction(EAction::MoveDown, EPadButton::DPadDown);

		/* Analog axes (up = +1, GLFW pad Y is inverted, so scale -1). */
		BindAxis(EAxis::MoveX, EKey::A, -1.0f);
		BindAxis(EAxis::MoveX, EKey::D, 1.0f);
		BindAxis(EAxis::MoveY, EKey::S, -1.0f);
		BindAxis(EAxis::MoveY, EKey::W, 1.0f);
		BindAxis(EAxis::MoveX, EPadAxis::LeftStickX, 1.0f);
		BindAxis(EAxis::MoveY, EPadAxis::LeftStickY, -1.0f);
		BindAxis(EAxis::AimX, EPadAxis::RightStickX, 1.0f);
		BindAxis(EAxis::AimY, EPadAxis::RightStickY, -1.0f);
	}

	void CInputSystem::PollGamepads()
	{
		ConnectedPadCount = 0;
		for (std::int32_t Jid = 0; Jid < MAX_PADS; Jid++) {
			FPadState& Pad = Pads[Jid];
			const bool IsGamepad = (glfwJoystickIsGamepad(Jid) == GLFW_TRUE);

			if (IsGamepad && !Pad.bConnected) {
				Pad.bConnected = true;
				const char* Name = glfwGetGamepadName(Jid);
				LK_DEBUG_TAG("InputSystem", R"(Gamepad connected: id={} name="{}")", Jid, Name ? Name : "");
				OnPadConnected.Broadcast(Jid);
			} else if (!IsGamepad && Pad.bConnected) {
				Pad.bConnected = false;
				Pad.Buttons.fill(0);
				Pad.Axes.fill(0.0f);
				LK_DEBUG_TAG("InputSystem", "Pad disconnected: id={}", Jid);
				OnPadDisconnected.Broadcast(Jid);
				continue;
			}

			if (!IsGamepad) {
				continue;
			}

			ConnectedPadCount++;

			GLFWgamepadstate State{};
			if (glfwGetGamepadState(Jid, &State) != GLFW_TRUE) {
				continue;
			}

			for (std::size_t Idx = 0; Idx < PAD_BUTTON_COUNT; Idx++) {
				Pad.Buttons.at(Idx) = State.buttons[Idx];
			}
			for (std::size_t Idx = 0; Idx < PAD_AXIS_COUNT; Idx++) {
				Pad.Axes.at(Idx) = State.axes[Idx];
			}
		}
	}

	void CInputSystem::RefreshActionStates()
	{
		for (const EAction Action : Enum::View<EAction>()) {
			if (Action == EAction::None) {
				continue;
			}
			FActionRuntime& Runtime = Actions[std::to_underlying(Action)];
			Runtime.bDownLastFrame = Runtime.bDown;
			Runtime.bDown = ResolveActionDown(Action);

			if (Runtime.bDown && !Runtime.bDownLastFrame) {
				LK_TRACE_TAG("InputSystem", "Action pressed: {}", Action);
				OnActionPressed.Broadcast(Action);
			} else if (!Runtime.bDown && Runtime.bDownLastFrame) {
				LK_TRACE_TAG("InputSystem", "Action released: {}", Action);
				OnActionReleased.Broadcast(Action);
			}
		}
	}

	void CInputSystem::RefreshAxisValues()
	{
		for (const EAxis Axis : Enum::View<EAxis>()) {
			if (Axis == EAxis::None) {
				continue;
			}

			FAxisRuntime& Runtime = Axes[std::to_underlying(Axis)];
			Runtime.OldValue = Runtime.Value;
			Runtime.Value = ResolveAxisValue(Axis);
			if (std::abs(Runtime.Value - Runtime.OldValue) > AXIS_CHANGE_EPSILON) {
				OnAxisChanged.Broadcast(Axis, Runtime.Value);
			}
		}
	}

	bool CInputSystem::ResolveActionDown(const EAction Action) const
	{
		const FActionBindings& Binding = ActionBindings[std::to_underlying(Action)];

		for (const EKey Key : Binding.Keys) {
			if (CKeyboard::IsKeyDown(Key)) {
				return true;
			}
		}

		for (const EMouseButton Button : Binding.MouseButtons) {
			if (CMouse::IsDown(Button)) {
				return true;
			}
		}

		for (const EPadButton Button : Binding.PadButtons) {
			const std::size_t ButtonIdx = std::to_underlying(Button);
			if (ButtonIdx >= PAD_BUTTON_COUNT) {
				continue;
			}
			for (const FPadState& Pad : Pads) {
				if (Pad.bConnected && (Pad.Buttons[ButtonIdx] == GLFW_PRESS)) {
					return true;
				}
			}
		}

		return false;
	}

	float CInputSystem::ResolveAxisValue(const EAxis Axis) const
	{
		const FAxisBindings& Binding = AxisBindings[std::to_underlying(Axis)];

		float Value = 0.0f;
		for (const FKeyAxisBinding& Bind : Binding.Keys) {
			if (CKeyboard::IsKeyDown(Bind.Key)) {
				Value = CombineContribution(Value, Bind.Scale);
			}
		}

		for (const FPadAxisBinding& Bind : Binding.PadAxes) {
			const std::size_t AxisIdx = std::to_underlying(Bind.Axis);
			if (AxisIdx >= PAD_AXIS_COUNT) {
				continue;
			}

			const bool IsTrigger = (Bind.Axis == EPadAxis::LeftTrigger) || (Bind.Axis == EPadAxis::RightTrigger);
			for (const FPadState& Pad : Pads) {
				if (!Pad.bConnected) {
					continue;
				}

				float Raw = Pad.Axes[AxisIdx];
				if (IsTrigger) {
					Raw = TriggerToUnit(Raw);
				}

				const float Shaped = ApplyDeadZone(Raw, Bind.DeadZone) * Bind.Scale;
				Value = CombineContribution(Value, Shaped);
			}
		}

		return std::clamp(Value, -1.0f, 1.0f);
	}

	float CInputSystem::ApplyDeadZone(const float Value, const float DeadZone)
	{
		const float Abs = std::abs(Value);
		if (Abs < DeadZone) {
			return 0.0f;
		}

		const float Scaled = (Abs - DeadZone) / (1.0f - DeadZone);
		return (Value < 0.0f) ? -Scaled : Scaled;
	}

	float CInputSystem::TriggerToUnit(const float RawValue)
	{
		return (RawValue + 1.0f) * 0.5f;
	}

	float CInputSystem::CombineContribution(const float Current, const float Contribution)
	{
		return Current + Contribution;
	}

}
