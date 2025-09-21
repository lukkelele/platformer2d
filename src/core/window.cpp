#include "window.h"

#include <stdio.h>

namespace platformer2d {

	CWindow::CWindow(const uint16_t InWidth, const uint16_t InHeight)
		: Data(InWidth, InHeight)
	{
	}

	void CWindow::Initialize()
	{
		const int GlfwInit = glfwInit();
		glfwSetErrorCallback([](const int Error, const char* Description)
		{
			std::printf("GLFW error (%d): %s\n", Error, Description);
		});

		GlfwWindow = glfwCreateWindow(Data.Width, Data.Height, Data.Title.c_str(), nullptr, nullptr);
		glfwMakeContextCurrent(GlfwWindow);
	}

	void CWindow::Destroy()
	{
		glfwTerminate();
	}

}
