#include "test_base.h"

#include <string>
#include <stdexcept>

#if defined(_WIN32)
#include <Windows.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include "core/log.h"
#include "renderer/opengl.h"
#include "renderer/imguilayer.h"

#ifndef LK_TEST_SUITE
#error "LK_TEST_SUITE not defined"
#endif

#ifndef LK_TEST_NAME
#error "LK_TEST_NAME not defined"
#endif

namespace platformer2d::test {

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

		#define UI_COMBO_OPTION(Value) { Value, #Value }
		std::pair<GLenum, const char*> SourceBlendFuncs[] = {
			UI_COMBO_OPTION(GL_SRC_ALPHA),
			UI_COMBO_OPTION(GL_DST_ALPHA),
			UI_COMBO_OPTION(GL_SRC_ALPHA),
			UI_COMBO_OPTION(GL_ONE),
			UI_COMBO_OPTION(GL_ONE_MINUS_CONSTANT_ALPHA),
		};

		std::pair<GLenum, const char*> DestBlendFuncs[] = {
			UI_COMBO_OPTION(GL_SRC_ALPHA),
			UI_COMBO_OPTION(GL_DST_ALPHA),
			UI_COMBO_OPTION(GL_ONE_MINUS_SRC_ALPHA),
			UI_COMBO_OPTION(GL_ONE_MINUS_DST_ALPHA),
			UI_COMBO_OPTION(GL_ONE_MINUS_CONSTANT_ALPHA),
		};
	}

	static inline std::filesystem::path GetBinaryDir()
	{
#if defined(_WIN32)
		wchar_t Buffer[MAX_PATH];
		const DWORD Length = GetModuleFileNameW(nullptr, Buffer, MAX_PATH);
		if ((Length == 0) || (Length == MAX_PATH))
		{
			throw std::runtime_error("Failed to get executable path");
		}
		return std::filesystem::path(Buffer).parent_path();
#elif defined(__linux__)
		char Buffer[4096];
		ssize_t Count = readlink("/proc/self/exe", Buffer, sizeof(Buffer) - 1);
		if (Count == -1)
		{
			throw std::runtime_error("Failed to read /proc/self/exe");
		}
		Buffer[Count] = '\0';
		return std::filesystem::path(Buffer).parent_path();
#else
#error "Unsupported platform"
#endif
	}

	CTestBase::CTestBase(int Argc, char* Argv[], const bool bInit)
		: Args(Argc, Argv)
		, BinaryDir(GetBinaryDir())
	{
		if (bInit)
		{
			CLog::Initialize();
			LK_INFO("{}", LK_TEST_NAME);
			LK_TRACE("Binary dir: {}", BinaryDir.generic_string());
			LK_TRACE("Assets dir: {}", AssetsDir.generic_string());

			Window = std::make_unique<CWindow>(SCREEN_WIDTH, SCREEN_HEIGHT, LK_TEST_NAME);
			Window->Initialize();
			CImGuiLayer::AddViewportFlags(ImGuiWindowFlags_MenuBar);
		}
	}

	void CTestBase::Stop()
	{
		Running = false;
	}

	void CTestBase::InitRenderContext(GLFWwindow* GlfwWindow)
	{
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

		CImGuiLayer::SetDarkTheme();

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
		LK_ASSERT(Font, "Failed to load font");
	}

	void CTestBase::ImGui_NewFrame()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuiViewport* Viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(Viewport->Pos);
		ImGui::SetNextWindowSize(Viewport->Size);
		ImGui::SetNextWindowViewport(Viewport->ID);
		
		static constexpr int Flags = ImGuiWindowFlags_NoDecoration 
			| ImGuiWindowFlags_NoScrollbar 
			| ImGuiWindowFlags_NoBackground;
		ImGui::Begin(LK_TEST_NAME, NULL, Flags); /* LK_TEST_SUITE */
	}

	void CTestBase::ImGui_EndFrame()
	{
		ImGui::End(); /* LK_TEST_SUITE */
		ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	bool CTestBase::UI_BlendFunction()
	{
		static constexpr float ItemWidth = 380.0f;
		bool bSetBlendFunc = false;

		static int SelectedSourceBlendFunc = 0;
		ImGui::SetNextItemWidth(ItemWidth);
		if (ImGui::BeginCombo("Source", SourceBlendFuncs[SelectedSourceBlendFunc].second))
		{
			for (int N = 0; N < LK_ARRAYSIZE(SourceBlendFuncs); N++)
			{
				const bool bSelected = (SelectedSourceBlendFunc == N);
				if (ImGui::Selectable(SourceBlendFuncs[N].second, bSelected))
				{
					SelectedSourceBlendFunc = N;
					LK_INFO("Source: {}", SourceBlendFuncs[N].second);
					bSetBlendFunc = true;
				}
			}
			ImGui::EndCombo();
		}

		static int SelectedDestBlendFunc = 0;
		ImGui::SetNextItemWidth(ItemWidth);
		if (ImGui::BeginCombo("Destination", DestBlendFuncs[SelectedDestBlendFunc].second))
		{
			for (int N = 0; N < LK_ARRAYSIZE(DestBlendFuncs); N++)
			{
				const bool bSelected = (SelectedDestBlendFunc == N);
				if (ImGui::Selectable(DestBlendFuncs[N].second, bSelected))
				{
					SelectedDestBlendFunc = N;
					LK_INFO("Destination: {}", DestBlendFuncs[N].second);
					bSetBlendFunc = true;
				}
			}
			ImGui::EndCombo();
		}

		if (bSetBlendFunc)
		{
			glBlendFunc(
				SourceBlendFuncs[SelectedSourceBlendFunc].first,
				DestBlendFuncs[SelectedDestBlendFunc].first
			);
		}

		return bSetBlendFunc;
	}

}