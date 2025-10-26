#include "mouse.h"

#include "core/window.h"

namespace platformer2d {

	namespace
	{
		GLFWwindow* ActiveWindow = nullptr;
	}

	void CMouse::Initialize()
	{
		ActiveWindow = CWindow::Get()->GetGlfwWindow();
		LK_VERIFY(ActiveWindow);
	}

	void CMouse::Enable()
	{
	}

	void CMouse::Disable()
	{
	}

	FMouseButtonData& CMouse::UpdateButtonState(const EMouseButton Button, const EMouseButtonState NewState)
	{
		FMouseButtonData& Data = ButtonDataMap[Button];
		Data.Button = Button;
		Data.OldState = Data.State;
		Data.State = NewState;

		OnMouseButtonPressed.Broadcast(Data);

		return Data;
	}

	void CMouse::UpdateScrollState(const EMouseScrollDirection Direction)
	{
		OnMouseScrolled.Broadcast(Direction);
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
