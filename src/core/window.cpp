#include "window.h"

#include <stdio.h>

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include "renderer/opengl.h"

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
		LK_VERIFY(GlfwWindow);
		glfwMakeContextCurrent(GlfwWindow);

		const GLenum GladInitResult = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		LK_OpenGL_Verify(glEnable(GL_LINE_SMOOTH));
		LK_OpenGL_Verify(glEnable(GL_BLEND));

		/* Initialize ImGui. */
		ImGui::CreateContext();
		ImGuiIO& IO = ImGui::GetIO();
		IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		IO.ConfigDockingAlwaysTabBar = false;

		ImGui_ImplGlfw_InitForOpenGL(GlfwWindow, true);
		ImGui_ImplOpenGL3_Init("#version 450");
		std::printf("ImGui Version: %s\n", ImGui::GetVersion());
	}

	void CWindow::Destroy()
	{
		glfwTerminate();
	}

	void CWindow::BeginFrame()
	{
		glfwPollEvents();
		LK_OpenGL_Verify(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
		LK_OpenGL_Verify(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

		ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
	}

	void CWindow::EndFrame()
	{
		ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(GlfwWindow);
		glfwPollEvents();
	}

}
