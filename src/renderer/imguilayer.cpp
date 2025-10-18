#include "imguilayer.h"

#include <GLFW/glfw3.h>
#include <imgui/imgui_internal.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <spdlog/spdlog.h>

#include "core/core.h"
#include "core/log.h"

namespace platformer2d {

	namespace PanelID 
	{
		static const char* const Viewport  = "##Viewport";
		static const char* const Dockspace = "##Dockspace";
	}

	namespace
	{
		constexpr ImGuiWindowFlags CoreViewportFlags = ImGuiWindowFlags_NoTitleBar
			| ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar
			| ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus
			| ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDocking
			| ImGuiWindowFlags_NoBackground;

		constexpr ImGuiWindowFlags HostWindowFlags = ImGuiWindowFlags_NoTitleBar
			| ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus
			| ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoInputs
			| ImGuiWindowFlags_NoBackground;

		constexpr ImGuiDockNodeFlags DockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode
			| ImGuiDockNodeFlags_NoDockingInCentralNode;
	}

	CImGuiLayer::CImGuiLayer(GLFWwindow* InContext)
	{
		LK_ASSERT(InContext);

		ImGui::CreateContext();
		ImGuiIO& IO = ImGui::GetIO();
		IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		IO.ConfigDockingAlwaysTabBar = false;

		ImGui_ImplGlfw_InitForOpenGL(InContext, true);
		ImGui_ImplOpenGL3_Init("#version 450");
		spdlog::info("ImGui Version: {}", ImGui::GetVersion());

		SetDarkTheme();

		/** @todo Use generated header */
		const char* SourceSansPro_Semibold = FONTS_DIR "/SourceCodePro/SourceSansPro-Semibold.ttf";

		/* Add fonts. */
		ImFontConfig FontConfig;
		ImFont* Font = IO.Fonts->AddFontFromFileTTF(
			SourceSansPro_Semibold,
			22.0f,
			&FontConfig,	
			(FontConfig.GlyphRanges == nullptr ? IO.Fonts->GetGlyphRangesDefault() : FontConfig.GlyphRanges)
		);
		LK_ASSERT(Font, "Failed to load font: {}", SourceSansPro_Semibold);
	}

	CImGuiLayer::~CImGuiLayer()
	{
	}

	void CImGuiLayer::BeginFrame()
	{
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
			
			ImGui::DockBuilderFinish(DockspaceID);
		}

		ImGui::DockSpace(DockspaceID, ImVec2(0, 0), DockspaceFlags);
		/* Submit the dockspace. */
		ImGui::End(); /* Viewport */
#endif

		/* @todo Use dockspace approach instead. */
		static constexpr int Flags = ImGuiWindowFlags_NoDecoration 
			| ImGuiWindowFlags_NoScrollbar 
			| ImGuiWindowFlags_NoBackground;
		ImGui::Begin(PanelID::Viewport, NULL, Flags);
	}

	void CImGuiLayer::EndFrame()
	{
		ImGui::End(); /* Viewport */

		ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	void CImGuiLayer::SetDarkTheme()
    {
		ImGui::StyleColorsDark();
		auto& Colors = ImGui::GetStyle().Colors;

		/* Window background. */
		Colors[ImGuiCol_WindowBg]		= ImVec4(0.10f, 0.105f, 0.110f, 1.0f);

		/* Headers. */
		Colors[ImGuiCol_Header]			= ImVec4(0.20f, 0.205f,  0.210f, 1.0f);
		Colors[ImGuiCol_HeaderHovered]	= ImVec4(0.30f, 0.305f,  0.310f, 1.0f);
		Colors[ImGuiCol_HeaderActive]	= ImVec4(0.15f, 0.1505f, 0.151f, 1.0f);

		/* Buttons. */
		Colors[ImGuiCol_Button]			= ImVec4(0.20f, 0.205f, 0.210f, 1.0f);
		Colors[ImGuiCol_ButtonHovered]	= ImVec4(0.30f, 0.305f, 0.310f, 1.0f);
		Colors[ImGuiCol_ButtonActive]	= ImVec4(0.15f, 0.150f, 0.151f, 1.0f);

		/* Frame background. */
		Colors[ImGuiCol_FrameBg]		= ImVec4(0.20f, 0.2050f, 0.210f, 1.0f);
		Colors[ImGuiCol_FrameBgHovered]	= ImVec4(0.30f, 0.3050f, 0.310f, 1.0f);
		Colors[ImGuiCol_FrameBgActive]	= ImVec4(0.15f, 0.1505f, 0.151f, 1.0f);

		/* Tabs. */
		Colors[ImGuiCol_Tab]				= ImVec4(0.15f, 0.1505f, 0.151f, 1.0f);
		Colors[ImGuiCol_TabHovered]			= ImVec4(0.38f, 0.3805f, 0.381f, 1.0f);
		Colors[ImGuiCol_TabActive]			= ImVec4(0.28f, 0.2805f, 0.281f, 1.0f);
		Colors[ImGuiCol_TabUnfocused]		= ImVec4(0.15f, 0.1505f, 0.151f, 1.0f);
		Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.20f, 0.2050f, 0.210f, 1.0f);

		/* Title. */
		Colors[ImGuiCol_TitleBg]		  = ImVec4(0.150f, 0.1505f, 0.151f, 1.0f);
		Colors[ImGuiCol_TitleBgActive]	  = ImVec4(0.150f, 0.1505f, 0.151f, 1.0f);
		Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.150f, 0.1505f, 0.151f, 1.0f);

		/* Scrollbar. */
		Colors[ImGuiCol_ScrollbarBg]		  = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		Colors[ImGuiCol_ScrollbarGrab]		  = ImVec4(0.31f, 0.31f, 0.31f, 1.0f);
		Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.0f);
		Colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.51f, 0.51f, 0.51f, 1.0f);

		/* Checkmark. */
		Colors[ImGuiCol_CheckMark] = ImVec4(0.94f, 0.94f, 0.94f, 1.0f);

		/* Slider. */
		Colors[ImGuiCol_SliderGrab]		  = ImVec4(0.51f, 0.51f, 0.51f, 0.7f);
		Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.66f, 0.66f, 0.66f, 1.0f);
	}

}
