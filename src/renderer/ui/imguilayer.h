#pragma once

#include <cstdint>

#include <imgui/imgui.h>

#include "renderer/ui/imgui.h"

struct GLFWwindow;

namespace platformer2d {

	class CImGuiLayer
	{
	public:
		CImGuiLayer(GLFWwindow* InContext);
		CImGuiLayer() = delete;
		~CImGuiLayer() = default;

		void Destroy();

		void BeginFrame();
		void EndFrame();

		static void AddViewportFlags(ImGuiWindowFlags Flags);
		static void RemoveViewportFlags(ImGuiWindowFlags Flags);
		static void AddFonts();

		static void SetDarkTheme();

	private:
		void OnWindowResized(uint16_t InWidth, uint16_t InHeight);
	};

}
