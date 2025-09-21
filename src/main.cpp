#include <stdio.h>

#include "core/window.h"

int main(int argc, char* argv[])
{
	std::printf("platformer2d\n");

	platformer2d::CWindow Window(800, 600);
	Window.Initialize();

	while (true)
	{
		glfwSwapBuffers(Window.GlfwWindow);
		glfwPollEvents();
	}

	Window.Destroy();

	return 0;
}