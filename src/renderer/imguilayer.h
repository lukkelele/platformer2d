#pragma once

#include <imgui/imgui.h>

struct GLFWwindow;

namespace platformer2d {

	class CImGuiLayer
	{
	public:
		CImGuiLayer(GLFWwindow* InContext);
		CImGuiLayer() = delete;
		~CImGuiLayer();

		void BeginFrame();
		void EndFrame();

		void SetDarkTheme();
	};

}