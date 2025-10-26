#include "window.h"

#include <stdio.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <spdlog/spdlog.h>

#include "renderer/opengl.h"
#include "input/keyboard.h"
#include "input/mouse.h"

namespace platformer2d {

	static CWindow* Instance = NULL;

	CWindow::CWindow(const uint16_t InWidth, const uint16_t InHeight, std::string_view InTitle)
		: Data(InWidth, InHeight, InTitle)
	{
		Instance = this;
	}

	CWindow* CWindow::Get()
	{
		return Instance;
	}

	void CWindow::Initialize()
	{
		const int GlfwInit = glfwInit();
		glfwSetErrorCallback([](const int Error, const char* Description)
		{
			LK_ERROR("GLFW error ({}): {}", Error, Description);
		});

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, LK_OPENGL_MAJOR);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, LK_OPENGL_MINOR);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef LK_BUILD_DEBUG
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

		GlfwWindow = glfwCreateWindow(Data.Width, Data.Height, Data.Title.c_str(), nullptr, nullptr);
		LK_VERIFY(GlfwWindow);
		glfwMakeContextCurrent(GlfwWindow);
		glfwSetWindowUserPointer(GlfwWindow, &Data);

		glfwSetWindowSizeCallback(GlfwWindow, [](GLFWwindow* InGlfwWindow, int NewWidth, int NewHeight) 
		{
			CWindow* Window = CWindow::Get();
			LK_ASSERT(Window);
			Window->Data.Width = NewWidth;
			Window->Data.Height = NewHeight;
		});

		glfwSetWindowCloseCallback(GlfwWindow, [](GLFWwindow* InGlfwWindow)
		{
			spdlog::info("Closing window");
			glfwSetWindowShouldClose(InGlfwWindow, GLFW_TRUE);
			std::exit(0); /** @todo Should do proper shutdown */
		});

		SetVSync(true);

		glfwSetKeyCallback(GlfwWindow, [](GLFWwindow* Window, int Key, int ScanCode, int Action, int Modifiers)
		{
			LK_TRACE("Key={} Action={} Modifiers={}", Key, Action, Modifiers);
			FWindowData& WindowDataRef = *((FWindowData*)glfwGetWindowUserPointer(Window));
			switch (Action)
			{
				case GLFW_PRESS:
				{
					CKeyboard::UpdateKeyState(static_cast<EKey>(Key), EKeyState::Pressed);
					break;
				}
				case GLFW_RELEASE:
				{
					CKeyboard::UpdateKeyState(static_cast<EKey>(Key), EKeyState::Released);
					break;
				}
				case GLFW_REPEAT:
				{
					CKeyboard::UpdateKeyState(static_cast<EKey>(Key), EKeyState::Held);
					break;
				}
			}
		});

		glfwSetMouseButtonCallback(GlfwWindow, [](GLFWwindow* Window, int Button, int Action, int Modifiers)
		{
			switch (Action)
			{
				case GLFW_PRESS:
				{
					CMouse::UpdateButtonState(static_cast<EMouseButton>(Button), EMouseButtonState::Pressed);
					break;
				}
				case GLFW_RELEASE:
				{
					CMouse::UpdateButtonState(static_cast<EMouseButton>(Button), EMouseButtonState::Released);
					break;
				}
			}
		});

		glfwSetScrollCallback(GlfwWindow, [](GLFWwindow* Window, double OffsetX, double OffsetY)
		{
			if (OffsetY > 0)
			{
				CMouse::UpdateScrollState(EMouseScrollDirection::Up);
			}
			else if (OffsetY < 0)
			{
				CMouse::UpdateScrollState(EMouseScrollDirection::Down);
			}
		});
	}

	void CWindow::Destroy()
	{
		glfwTerminate();
	}

	void CWindow::BeginFrame()
	{
		glfwPollEvents();
	}

	void CWindow::EndFrame()
	{
		glfwSwapBuffers(GlfwWindow);
		glfwPollEvents();
	}

	void CWindow::SetVSync(const bool Enabled)
	{
		LK_DEBUG_TAG("Window", "VSync: {}", Enabled ? "enabled" : "disabled");
		glfwSwapInterval(Enabled);
		Instance->Data.bVSync = Enabled;
	}

}
