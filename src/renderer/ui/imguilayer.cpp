#include "imguilayer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui/imgui_internal.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <ImGuizmo/ImGuizmo.h>

#include "core/core.h"
#include "core/log.h"
#include "core/window.h"
#include "renderer/font.h"
#include "scoped.h"
#include "ui_core.h"

namespace platformer2d {

	CImGuiLayer::CImGuiLayer(GLFWwindow* InContext)
	{
		LK_VERIFY(InContext);
		ImGui::CreateContext();
		ImGuiIO& IO = ImGui::GetIO();
		IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		IO.ConfigDockingAlwaysTabBar = false;

		/** @fixme: Need to sort out the initial dock node placements */
#define NO_IMGUI_CONFIG 1
#if NO_IMGUI_CONFIG
		IO.IniFilename = nullptr; /* No config. */
#endif

		ImGui_ImplGlfw_InitForOpenGL(InContext, true);
		ImGui_ImplOpenGL3_Init("#version 460");
		LK_INFO("ImGui: {}", ImGui::GetVersion());

		AddFonts();
		SetDarkTheme();

		CWindow::OnResized.Add(this, &CImGuiLayer::OnWindowResized);
	}

	void CImGuiLayer::Destroy()
	{
		LK_TRACE_TAG("ImGuiLayer", "Destroy");
		ImGui_ImplGlfw_Shutdown();
		ImGui_ImplOpenGL3_Shutdown();
		ImGui::DestroyContext();
	}

	void CImGuiLayer::BeginFrame()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();

		UI::BeginViewport(CWindow::Get());
	}

	void CImGuiLayer::EndFrame()
	{
		ImGui::End(); /* Viewport */

		ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	void CImGuiLayer::AddViewportFlags(const ImGuiWindowFlags Flags)
	{
		UI::ViewportFlags |= Flags;
	}

	void CImGuiLayer::RemoveViewportFlags(const ImGuiWindowFlags Flags)
	{
		UI::ViewportFlags &= ~Flags;
	}

	void CImGuiLayer::AddFonts()
	{
		ImGuiIO& IO = ImGui::GetIO();

		/********************
		 * FontAwesome
		 ********************/
		constexpr ImWchar FontAwesomeRanges[] = { LK_ICON_MIN, LK_ICON_MAX, 0 };
		FFontConfiguration FontAwesome;
		FontAwesome.Font = EFont::FontAwesome;
		FontAwesome.Size = EFontSize::Regular;
		FontAwesome.FilePath = FONTS_DIR "/FontAwesome/fontawesome-webfont.ttf";
		FontAwesome.GlyphRanges = FontAwesomeRanges;
		FontAwesome.MergeWithLast = true;

		auto AddFont = [&FontAwesome](FFontConfiguration& Config, const bool ScaleFontAwesome = true) -> void
		{
			const EFontSize FontAwesomeSizeCopy = FontAwesome.Size;
			for (int Idx = 0; Idx < static_cast<int>(EFontSize::COUNT); Idx++)
			{
				const EFontSize FontSize = static_cast<EFontSize>(Idx);
				if (FontSize == EFontSize::COUNT)
				{
					break;
				}

				if (ScaleFontAwesome)
				{
					if (static_cast<int>(FontSize) >= static_cast<int>(EFontSize::Large))
					{
						FontAwesome.Size = EFontSize::Large;
					}
				}

				Config.Size = FontSize;
				UI::Font::Add(Config);
				UI::Font::Add(FontAwesome);
			}

			FontAwesome.Size = FontAwesomeSizeCopy;
		};


		/********************
		 * Source Sans Pro
		 ********************/
		{
			FFontConfiguration SourceSansPro_Semibold;
			SourceSansPro_Semibold.Font = EFont::SourceSansPro;
			SourceSansPro_Semibold.Size = EFontSize::Regular;
			SourceSansPro_Semibold.Modifier = EFontModifier::Normal;
			SourceSansPro_Semibold.FilePath = FONTS_DIR "/SourceCodePro/SourceSansPro-Semibold.ttf";

			AddFont(SourceSansPro_Semibold);
		}

		{
			FFontConfiguration Conf;
			Conf.Font = EFont::SourceSansPro;
			Conf.Size = EFontSize::Regular;
			Conf.Modifier = EFontModifier::Bold;
			Conf.FilePath = FONTS_DIR "/SourceCodePro/SourceSansPro-Bold.ttf";
			AddFont(Conf);
		}

		/* Italic */
		{
			FFontConfiguration Conf;
			Conf.Font = EFont::SourceSansPro;
			Conf.Size = EFontSize::Regular;
			Conf.Modifier = EFontModifier::Italic;
			Conf.FilePath = FONTS_DIR "/SourceCodePro/SourceSansPro-Italic.ttf";
			AddFont(Conf);
		}

		/* Bold Italic */
		{
			FFontConfiguration Conf;
			Conf.Font = EFont::SourceSansPro;
			Conf.Size = EFontSize::Regular;
			Conf.Modifier = EFontModifier::BoldItalic;
			Conf.FilePath = FONTS_DIR "/SourceCodePro/SourceSansPro-BoldItalic.ttf";
			AddFont(Conf);
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

			AddFont(Roboto_Regular);
		}

		{
			FFontConfiguration Roboto_Bold;
			Roboto_Bold.Font = EFont::Roboto;
			Roboto_Bold.Size = EFontSize::Regular;
			Roboto_Bold.Modifier = EFontModifier::Bold;
			Roboto_Bold.FilePath = FONTS_DIR "/Roboto/Roboto-Bold.ttf";

			AddFont(Roboto_Bold);
		}

		{
			FFontConfiguration Roboto_SemiMedium;
			Roboto_SemiMedium.Font = EFont::Roboto;
			Roboto_SemiMedium.Size = EFontSize::Regular;
			Roboto_SemiMedium.Modifier = EFontModifier::SemiMedium;
			Roboto_SemiMedium.FilePath = FONTS_DIR "/Roboto/Roboto-SemiMedium.ttf";

			AddFont(Roboto_SemiMedium);
		}

		{
			FFontConfiguration Roboto_Italic;
			Roboto_Italic.Font = EFont::Roboto;
			Roboto_Italic.Size = EFontSize::Regular;
			Roboto_Italic.Modifier = EFontModifier::Italic;
			Roboto_Italic.FilePath = FONTS_DIR "/Roboto/Roboto-BoldItalic.ttf";

			AddFont(Roboto_Italic);
		}

		{
			FFontConfiguration Roboto_BoldItalic;
			Roboto_BoldItalic.Font = EFont::Roboto;
			Roboto_BoldItalic.Size = EFontSize::Regular;
			Roboto_BoldItalic.Modifier = EFontModifier::BoldItalic;
			Roboto_BoldItalic.FilePath = FONTS_DIR "/Roboto/Roboto-BoldItalic.ttf";

			AddFont(Roboto_BoldItalic);
		}
	}

	void CImGuiLayer::OnWindowResized(const uint16_t InWidth, const uint16_t InHeight)
	{
		ImGuiIO& IO = ImGui::GetIO();
		IO.DisplaySize = ImVec2(static_cast<float>(InWidth), static_cast<float>(InHeight));
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
