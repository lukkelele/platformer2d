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

	namespace PanelID {
		static const char* const Viewport  = "##Viewport";
		static const char* const Dockspace = "##Dockspace";
	}

	static constexpr ImGuiWindowFlags CoreViewportFlags = ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar
		| ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus
		| ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDocking
		| ImGuiWindowFlags_NoBackground;

	static constexpr ImGuiWindowFlags HostWindowFlags = ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus
		| ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoInputs
		| ImGuiWindowFlags_NoBackground;

	static constexpr ImGuiDockNodeFlags DockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode
		| ImGuiDockNodeFlags_NoDockingInCentralNode;

	CWindow::CWindow(const uint16_t InWidth, const uint16_t InHeight)
		: Data(InWidth, InHeight)
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
		spdlog::info("ImGui Version: {}", ImGui::GetVersion());
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

		ImGuiViewport* Viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(Viewport->Pos);
        ImGui::SetNextWindowSize(Viewport->Size);
        ImGui::SetNextWindowViewport(Viewport->ID);

#ifdef DOCKSPACE_ENABLED
		ImGui::Begin(PanelID::Viewport, NULL, HostWindowFlags);
		ImGuiID DockspaceID = ImGui::GetID(PanelID::Dockspace);
		if (ImGui::DockBuilderGetNode(DockspaceID) == nullptr)
		{
			spdlog::warn("Removing existing layout");
			/* Remove existing layout. */
			ImGui::DockBuilderRemoveNode(DockspaceID);
			ImGuiDockNodeFlags DockFlags = ImGuiDockNodeFlags_DockSpace 
				| ImGuiDockNodeFlags_NoWindowMenuButton;
			ImGui::DockBuilderAddNode(DockspaceID, DockFlags);
			ImGui::DockBuilderSetNodeSize(DockspaceID, Viewport->Size);

			ImGuiID DockID_Main = DockspaceID;
			
			/* Finish the dockspace. */
			ImGui::DockBuilderFinish(DockspaceID);
		}

		ImGui::DockSpace(DockspaceID, ImVec2(0, 0), DockspaceFlags);
		/* Submit the dockspace. */
		ImGui::End(); /* Viewport */
#endif

		ImGui::Begin(PanelID::Viewport, NULL, CoreViewportFlags);
	}

	void CWindow::EndFrame()
	{
		ImGui::End(); /* Viewport */

		ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(GlfwWindow);
		glfwPollEvents();
	}

}
