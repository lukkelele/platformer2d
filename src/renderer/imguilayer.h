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

		static void AddViewportFlags(ImGuiWindowFlags Flags);
		static void RemoveViewportFlags(ImGuiWindowFlags Flags);

		static void SetDarkTheme();

	private:
		void AddFonts();
	};

}