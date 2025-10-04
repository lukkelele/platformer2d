#include "window.h"

#include <stdio.h>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <spdlog/spdlog.h>

#include "renderer/opengl.h"

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
			spdlog::error("GLFW error ({}): {}", Error, Description);
		});

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

}
