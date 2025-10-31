#include "imguilayer.h"

#include <GLFW/glfw3.h>
#include <imgui/imgui_internal.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <spdlog/spdlog.h>

#include "core/core.h"
#include "core/log.h"
#include "font.h"

namespace platformer2d {

	namespace PanelID 
	{
		static const char* const Dockspace = "##Dockspace";
		static const char* const HostWindow = "##HostWindow";
		static const char* const Viewport  = "##Viewport";
	}

	namespace
	{
		ImGuiWindowFlags HostWindowFlags = ImGuiWindowFlags_NoTitleBar
			| ImGuiWindowFlags_NoCollapse
			| ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoNavFocus
			| ImGuiWindowFlags_NoInputs
			| ImGuiWindowFlags_NoBringToFrontOnFocus
			| ImGuiWindowFlags_NoBackground
			| ImGuiWindowFlags_NoSavedSettings;

		ImGuiWindowFlags ViewportFlags = ImGuiWindowFlags_NoTitleBar
			| ImGuiWindowFlags_NoCollapse
			| ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar
			| ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoBackground
			| ImGuiWindowFlags_NoBringToFrontOnFocus
			| ImGuiWindowFlags_NoDocking
			| ImGuiWindowFlags_NoSavedSettings;

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
		LK_INFO("ImGui Version: {}", ImGui::GetVersion());

		AddFonts();
		SetDarkTheme();
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

		ImGui::Begin(PanelID::HostWindow, NULL, HostWindowFlags);
		ImGuiID DockspaceID = ImGui::GetID(PanelID::Dockspace);
		if (ImGui::DockBuilderGetNode(DockspaceID) == nullptr)
		{
			LK_WARN("Removing existing layout");
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

#ifdef NO_WINDOW_PADDING
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
#endif
		ImGui::SetNextWindowPos(Viewport->Pos);
		ImGui::SetNextWindowSize(Viewport->Size);
		ImGui::SetNextWindowViewport(Viewport->ID);
		ImGui::Begin(PanelID::Viewport, NULL, ViewportFlags);
#ifdef NO_WINDOW_PADDING
		ImGui::PopStyleVar(2);
#endif
	}

	void CImGuiLayer::EndFrame()
	{
		ImGui::End(); /* Viewport */

		ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	void CImGuiLayer::AddViewportFlags(const ImGuiWindowFlags Flags)
	{
		ViewportFlags |= Flags;
	}

	void CImGuiLayer::RemoveViewportFlags(const ImGuiWindowFlags Flags)
	{
		ViewportFlags &= ~Flags;
	}

	void CImGuiLayer::AddFonts()
	{
		ImGuiIO& IO = ImGui::GetIO();

		/********************
		 * Source Sans Pro
		 ********************/
		{
			FFontConfiguration SourceSansPro_Semibold;
			SourceSansPro_Semibold.Font = EFont::SourceSansPro;
			SourceSansPro_Semibold.Size = EFontSize::Regular;
			SourceSansPro_Semibold.Modifier = EFontModifier::Normal;
			SourceSansPro_Semibold.FilePath = FONTS_DIR "/SourceCodePro/SourceSansPro-Semibold.ttf";
			UI::Font::Add(SourceSansPro_Semibold, true);

			/********************
			 * FontAwesome
			 *
			 * Merge with last.
			 ********************/
			constexpr ImWchar FontAwesomeRanges[] = { LK_ICON_MIN, LK_ICON_MAX, 0 };
			FFontConfiguration FontAwesome;
			FontAwesome.Font = EFont::FontAwesome;
			FontAwesome.Size = EFontSize::Regular;
			FontAwesome.FilePath = FONTS_DIR "/FontAwesome/fontawesome-webfont.ttf";
			FontAwesome.GlyphRanges = FontAwesomeRanges;
			FontAwesome.MergeWithLast = true;
			UI::Font::Add(FontAwesome);

			SourceSansPro_Semibold.Size = EFontSize::Smaller;
			UI::Font::Add(SourceSansPro_Semibold);

			SourceSansPro_Semibold.Size = EFontSize::Small;
			UI::Font::Add(SourceSansPro_Semibold);

			SourceSansPro_Semibold.Size = EFontSize::Large;
			UI::Font::Add(SourceSansPro_Semibold);

			SourceSansPro_Semibold.Size = EFontSize::Larger;
			UI::Font::Add(SourceSansPro_Semibold);

			SourceSansPro_Semibold.Size = EFontSize::Header;
			UI::Font::Add(SourceSansPro_Semibold);

			SourceSansPro_Semibold.Size = EFontSize::Title;
			UI::Font::Add(SourceSansPro_Semibold);
		}

		{
			FFontConfiguration SourceSansPro_Bold;
			SourceSansPro_Bold.Font = EFont::SourceSansPro;
			SourceSansPro_Bold.Size = EFontSize::Regular;
			SourceSansPro_Bold.Modifier = EFontModifier::Bold;
			SourceSansPro_Bold.FilePath = FONTS_DIR "/SourceCodePro/SourceSansPro-Bold.ttf";
			UI::Font::Add(SourceSansPro_Bold);

			SourceSansPro_Bold.Size = EFontSize::Smaller;
			UI::Font::Add(SourceSansPro_Bold);

			SourceSansPro_Bold.Size = EFontSize::Small;
			UI::Font::Add(SourceSansPro_Bold);

			SourceSansPro_Bold.Size = EFontSize::Large;
			UI::Font::Add(SourceSansPro_Bold);

			SourceSansPro_Bold.Size = EFontSize::Larger;
			UI::Font::Add(SourceSansPro_Bold);

			SourceSansPro_Bold.Size = EFontSize::Header;
			UI::Font::Add(SourceSansPro_Bold);

			SourceSansPro_Bold.Size = EFontSize::Title;
			UI::Font::Add(SourceSansPro_Bold);
		}

		{
			FFontConfiguration SourceSansPro_Italic;
			SourceSansPro_Italic.Font = EFont::SourceSansPro;
			SourceSansPro_Italic.Size = EFontSize::Regular;
			SourceSansPro_Italic.Modifier = EFontModifier::Italic;
			SourceSansPro_Italic.FilePath = FONTS_DIR "/SourceCodePro/SourceSansPro-Italic.ttf";
			UI::Font::Add(SourceSansPro_Italic);

			SourceSansPro_Italic.Size = EFontSize::Smaller;
			UI::Font::Add(SourceSansPro_Italic);

			SourceSansPro_Italic.Size = EFontSize::Small;
			UI::Font::Add(SourceSansPro_Italic);

			SourceSansPro_Italic.Size = EFontSize::Large;
			UI::Font::Add(SourceSansPro_Italic);

			SourceSansPro_Italic.Size = EFontSize::Larger;
			UI::Font::Add(SourceSansPro_Italic);

			SourceSansPro_Italic.Size = EFontSize::Header;
			UI::Font::Add(SourceSansPro_Italic);

			SourceSansPro_Italic.Size = EFontSize::Title;
			UI::Font::Add(SourceSansPro_Italic);
		}

		/********************
		 * Roboto
		 ********************/
		{
			FFontConfiguration Roboto_Regular;
			Roboto_Regular.Font = EFont::Roboto;
			Roboto_Regular.Size = EFontSize::Regular;
			Roboto_Regular.Modifier = EFontModifier::Normal;
			Roboto_Regular.FilePath = FONTS_DIR "/Roboto/Roboto-Regular.ttf";
			UI::Font::Add(Roboto_Regular);

			Roboto_Regular.Size = EFontSize::Smaller;
			UI::Font::Add(Roboto_Regular);

			Roboto_Regular.Size = EFontSize::Small;
			UI::Font::Add(Roboto_Regular);

			Roboto_Regular.Size = EFontSize::Large;
			UI::Font::Add(Roboto_Regular);

			Roboto_Regular.Size = EFontSize::Larger;
			UI::Font::Add(Roboto_Regular);

			Roboto_Regular.Size = EFontSize::Header;
			UI::Font::Add(Roboto_Regular);

			Roboto_Regular.Size = EFontSize::Title;
			UI::Font::Add(Roboto_Regular);
		}

		{
			FFontConfiguration Roboto_Bold;
			Roboto_Bold.Font = EFont::Roboto;
			Roboto_Bold.Size = EFontSize::Regular;
			Roboto_Bold.Modifier = EFontModifier::Bold;
			Roboto_Bold.FilePath = FONTS_DIR "/Roboto/Roboto-Bold.ttf";
			UI::Font::Add(Roboto_Bold);

			Roboto_Bold.Size = EFontSize::Smaller;
			UI::Font::Add(Roboto_Bold);

			Roboto_Bold.Size = EFontSize::Small;
			UI::Font::Add(Roboto_Bold);

			Roboto_Bold.Size = EFontSize::Large;
			UI::Font::Add(Roboto_Bold);

			Roboto_Bold.Size = EFontSize::Larger;
			UI::Font::Add(Roboto_Bold);

			Roboto_Bold.Size = EFontSize::Header;
			UI::Font::Add(Roboto_Bold);

			Roboto_Bold.Size = EFontSize::Title;
			UI::Font::Add(Roboto_Bold);
		}

		{
			FFontConfiguration Roboto_SemiMedium;
			Roboto_SemiMedium.Font = EFont::Roboto;
			Roboto_SemiMedium.Size = EFontSize::Regular;
			Roboto_SemiMedium.Modifier = EFontModifier::SemiMedium;
			Roboto_SemiMedium.FilePath = FONTS_DIR "/Roboto/Roboto-SemiMedium.ttf";
			UI::Font::Add(Roboto_SemiMedium);

			Roboto_SemiMedium.Size = EFontSize::Smaller;
			UI::Font::Add(Roboto_SemiMedium);

			Roboto_SemiMedium.Size = EFontSize::Small;
			UI::Font::Add(Roboto_SemiMedium);

			Roboto_SemiMedium.Size = EFontSize::Large;
			UI::Font::Add(Roboto_SemiMedium);

			Roboto_SemiMedium.Size = EFontSize::Larger;
			UI::Font::Add(Roboto_SemiMedium);

			Roboto_SemiMedium.Size = EFontSize::Header;
			UI::Font::Add(Roboto_SemiMedium);

			Roboto_SemiMedium.Size = EFontSize::Title;
			UI::Font::Add(Roboto_SemiMedium);
		}

		{
			FFontConfiguration Roboto_BoldItalic;
			Roboto_BoldItalic.Font = EFont::Roboto;
			Roboto_BoldItalic.Size = EFontSize::Regular;
			Roboto_BoldItalic.Modifier = EFontModifier::BoldItalic;
			Roboto_BoldItalic.FilePath = FONTS_DIR "/Roboto/Roboto-BoldItalic.ttf";
			UI::Font::Add(Roboto_BoldItalic);

			Roboto_BoldItalic.Size = EFontSize::Smaller;
			UI::Font::Add(Roboto_BoldItalic);

			Roboto_BoldItalic.Size = EFontSize::Small;
			UI::Font::Add(Roboto_BoldItalic);

			Roboto_BoldItalic.Size = EFontSize::Large;
			UI::Font::Add(Roboto_BoldItalic);

			Roboto_BoldItalic.Size = EFontSize::Larger;
			UI::Font::Add(Roboto_BoldItalic);

			Roboto_BoldItalic.Size = EFontSize::Header;
			UI::Font::Add(Roboto_BoldItalic);

			Roboto_BoldItalic.Size = EFontSize::Title;
			UI::Font::Add(Roboto_BoldItalic);
		}
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
