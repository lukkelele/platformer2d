#include <stdio.h>

#include <imgui/imgui.h>

#include "core/window.h"

int main(int argc, char* argv[])
{
	std::printf("platformer2d\n");

	platformer2d::CWindow Window(800, 600);
	Window.Initialize();

	while (true)
	{
		Window.BeginFrame();

		ImGui::Begin("Viewport");
		ImGui::Text("platformer2d");
		ImGui::End();

		Window.EndFrame();
	}

	Window.Destroy();

	return 0;
}