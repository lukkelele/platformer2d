#include "mouse.h"

#include "core/window.h"

namespace platformer2d {

	static GLFWwindow* ActiveWindow = nullptr;

	void CMouse::Initialize()
	{
		ActiveWindow = CWindow::Get().GetGlfwWindow();
		LK_VERIFY(ActiveWindow);

		for (std::size_t Idx = 0; Idx < std::to_underlying(EMouseButton::COUNT); Idx++) {
			ButtonDataMap[static_cast<EMouseButton>(Idx)] = {};
		}
	}

	void CMouse::Enable()
	{
	}

	void CMouse::Disable()
	{
	}

	bool CMouse::IsDown(const EMouseButton Button)
	{
		const EMouseButtonState State = ButtonDataMap.at(Button).State;
		return (State == EMouseButtonState::Pressed) || (State == EMouseButtonState::Held);
	}

	EMouseButtonState CMouse::GetState(const EMouseButton Button)
	{
		return ButtonDataMap.at(Button).State;
	}

	FMouseButtonData& CMouse::UpdateButtonState(const EMouseButton Button, const EMouseButtonState NewState)
	{
		FMouseButtonData& Data = ButtonDataMap.at(Button);
		Data.Button = Button;
		Data.OldState = Data.State;
		Data.State = NewState;

		OnButtonPressed.Broadcast(Data);
		return Data;
	}

	void CMouse::UpdateScrollState(const EMouseScrollDirection Direction)
	{
		OnScrolled.Broadcast(Direction);
	}

	float CMouse::GetX()
	{
		double X, Y;
		glfwGetCursorPos(ActiveWindow, &X, &Y);
		return static_cast<float>(X);
	}

	float CMouse::GetY()
	{
		double X, Y;
		glfwGetCursorPos(ActiveWindow, &X, &Y);
		return static_cast<float>(Y);
	}

	std::pair<float, float> CMouse::GetPos()
	{
		double X, Y;
		glfwGetCursorPos(ActiveWindow, &X, &Y);
		return std::make_pair<float, float>(X, Y);
	}

}
