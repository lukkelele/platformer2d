#include "test_base.h"

#include <string>
#include <stdexcept>

#if defined(_WIN32)
#include <Windows.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>

#include "core/log.h"
#include "renderer/opengl.h"
#include "renderer/ui/imguilayer.h"

#ifndef LK_TEST_SUITE
#error "LK_TEST_SUITE not defined"
#endif

#ifndef LK_TEST_NAME
#error "LK_TEST_NAME not defined"
#endif

namespace platformer2d::test {

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

	#define UI_COMBO_OPTION(Value) { Value, #Value }
	static std::pair<GLenum, const char*> SourceBlendFuncs[] = {
		UI_COMBO_OPTION(GL_SRC_ALPHA),
		UI_COMBO_OPTION(GL_DST_ALPHA),
		UI_COMBO_OPTION(GL_SRC_ALPHA),
		UI_COMBO_OPTION(GL_ONE),
		UI_COMBO_OPTION(GL_ONE_MINUS_CONSTANT_ALPHA),
	};

	static std::pair<GLenum, const char*> DestBlendFuncs[] = {
		UI_COMBO_OPTION(GL_SRC_ALPHA),
		UI_COMBO_OPTION(GL_DST_ALPHA),
		UI_COMBO_OPTION(GL_ONE_MINUS_SRC_ALPHA),
		UI_COMBO_OPTION(GL_ONE_MINUS_DST_ALPHA),
		UI_COMBO_OPTION(GL_ONE_MINUS_CONSTANT_ALPHA),
	};

	static std::filesystem::path GetBinaryDir()
	{
#if defined(_WIN32)
		wchar_t Buffer[MAX_PATH];
		const DWORD Length = GetModuleFileNameW(nullptr, Buffer, MAX_PATH);
		if ((Length == 0) || (Length == MAX_PATH)) {
			throw std::runtime_error("Failed to get executable path");
		}
		return std::filesystem::path(Buffer).parent_path();
#elif defined(__linux__)
		char Buffer[4096];
		ssize_t Count = readlink("/proc/self/exe", Buffer, sizeof(Buffer) - 1);
		if (Count == -1) {
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
		if (bInit) {
			lklog::init(lklog::level::trace);
			LK_INFO("{}", LK_TEST_NAME);
			LK_TRACE("Binary dir: {}", BinaryDir.generic_string());
			LK_TRACE("Assets dir: {}", AssetsDir.generic_string());

			const FWindowSpecification WindowSpec = {
				.Width = SCREEN_WIDTH,
				.Height = SCREEN_HEIGHT,
				.Title = LK_TEST_NAME,
				.bStartMaximized = true,
				.bVSync = true
			};
			Window = std::make_unique<CWindow>(WindowSpec);
			Window->Initialize();
			CImGuiLayer::AddViewportFlags(ImGuiWindowFlags_MenuBar);
		}
	}

	void CTestBase::Stop()
	{
		bRunning = false;
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

		CImGuiLayer::AddFonts();
		CImGuiLayer::SetDarkTheme();
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