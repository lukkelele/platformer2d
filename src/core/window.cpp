#include "window.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <stb/stb_image.h>
#include <spdlog/spdlog.h>

#include "renderer/opengl.h"
#include "profiler.h"
#include "input/keyboard.h"
#include "input/mouse.h"

namespace platformer2d {

	CWindow& CWindow::Get()
	{
		static CWindow Instance;
		return Instance;
	}

	void CWindow::Initialize(const FWindowSpecification& Spec)
	{
		Data.Width = Spec.Width;
		Data.Height = Spec.Height;
		Data.Title = Spec.Title;

		const int GlfwInit = glfwInit();
		glfwSetErrorCallback([](const int Error, const char* Description)
		{
			if (Error == GLFW_FEATURE_UNAVAILABLE) {
				LK_WARN_TAG("Window", "GLFW error ({}): {}", Error, Description);
			} else {
				LK_ERROR_TAG("Window", "GLFW error ({}): {}", Error, Description);
			}
		});

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, LK_OPENGL_MAJOR);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, LK_OPENGL_MINOR);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef LK_BUILD_DEBUG
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

		LK_DEBUG_TAG("Window", "Create: ({}, {})", Data.Width, Data.Height);
		Data.WindowRef = this;
		GlfwWindow = glfwCreateWindow(Data.Width, Data.Height, Data.Title.c_str(), nullptr, nullptr);
		LK_VERIFY(GlfwWindow);
		glfwMakeContextCurrent(GlfwWindow);
		glfwSetWindowUserPointer(GlfwWindow, &Data);

		SetVSync(Spec.bVSync);

		glfwSetWindowSizeCallback(GlfwWindow, [](GLFWwindow* InGlfwWindow, int NewWidth, int NewHeight)
		{
			FWindowData& Data = *((FWindowData*)glfwGetWindowUserPointer(InGlfwWindow));
			LK_ASSERT(Data.WindowRef, "Invalid window reference");
			Data.WindowRef->SetSize(NewWidth, NewHeight);
		});

		glfwSetWindowCloseCallback(GlfwWindow, [](GLFWwindow* InGlfwWindow)
		{
			LK_TRACE_TAG("Window", "Set close flag");
			glfwSetWindowShouldClose(InGlfwWindow, GLFW_TRUE);
		});

		glfwSetKeyCallback(GlfwWindow, [](GLFWwindow* Window, int Key, int ScanCode, int Action, int Modifiers)
		{
			LK_TRACE_TAG("Window", "Key={} Action={} Modifiers={}", Key, Action, Modifiers);
			FWindowData& WindowDataRef = *((FWindowData*)glfwGetWindowUserPointer(Window));
			switch (Action) {
				case GLFW_PRESS:
					CKeyboard::UpdateKeyState(static_cast<EKey>(Key), EKeyState::Pressed);
					break;
				case GLFW_RELEASE:
					CKeyboard::UpdateKeyState(static_cast<EKey>(Key), EKeyState::Released);
					break;
				case GLFW_REPEAT:
					CKeyboard::UpdateKeyState(static_cast<EKey>(Key), EKeyState::Held);
					break;
			}
		});

		glfwSetMouseButtonCallback(GlfwWindow, [](GLFWwindow* Window, const int Button, const int Action, const int Modifiers)
		{
			switch (Action) {
				case GLFW_PRESS:
					CMouse::UpdateButtonState(static_cast<EMouseButton>(Button), EMouseButtonState::Pressed);
					break;
				case GLFW_RELEASE:
					CMouse::UpdateButtonState(static_cast<EMouseButton>(Button), EMouseButtonState::Released);
					break;
				default:
					LK_WARN_TAG("Window", "Unhandled GLFW action: {} (Button={} Modifiers={})", Action, Button, Action);
					break;
			}
		});

		glfwSetScrollCallback(GlfwWindow, [](GLFWwindow* Window, double OffsetX, double OffsetY)
		{
			if (OffsetY > 0) {
				CMouse::UpdateScrollState(EMouseScrollDirection::Up);
			} else if (OffsetY < 0) {
				CMouse::UpdateScrollState(EMouseScrollDirection::Down);
			}
		});

		glfwSetWindowMaximizeCallback(GlfwWindow, [](GLFWwindow* Window, int Maximized)
		{
			LK_TRACE_TAG("Window", "Maximize callback");
			FWindowData& WindowData = *static_cast<FWindowData*>(glfwGetWindowUserPointer(Window));
			LK_UNUSED(WindowData);
		});

		glfwSetFramebufferSizeCallback(GlfwWindow, [](GLFWwindow* Window, int Width, int Height)
		{
			CWindow::OnFramebufferResized.Broadcast(Width, Height);
		});

		/* Set window icon. */
		const std::filesystem::path IconPath = TEXTURES_DIR "/test/test_player.png";
		SetIcon(IconPath);

		Centralize();
		if (Spec.bStartMaximized) {
			glfwMaximizeWindow(GlfwWindow);
		}
	}

	void CWindow::Destroy()
	{
		LK_TRACE_TAG("Window", "Destroy");
		glfwTerminate();
		GlfwWindow = nullptr;
	}

	void CWindow::BeginFrame()
	{
		LK_PROFILER_SCOPED();
		glfwPollEvents();
	}

	void CWindow::EndFrame()
	{
		LK_PROFILER_SCOPED();
		glfwSwapBuffers(GlfwWindow);
	}

	void CWindow::SetSize(const uint16_t InWidth, const uint16_t InHeight)
	{
		if ((Data.Width == InWidth) && (Data.Height == InHeight)) {
			return;
		}

		LK_TRACE_TAG("Window", "Resize: ({}, {})", InWidth, InHeight);
		Data.Width = InWidth;
		Data.Height = InHeight;

		if (GlfwWindow) {
			int CurrentW = 0;
			int CurrentH = 0;
			glfwGetWindowSize(GlfwWindow, &CurrentW, &CurrentH);
			if ((CurrentW != static_cast<int>(InWidth)) || (CurrentH != static_cast<int>(InHeight))) {
				if (IsMaximized()) {
					glfwRestoreWindow(GlfwWindow);
				}
				glfwSetWindowSize(GlfwWindow, InWidth, InHeight);
				Centralize();
			}
		}

		OnResized.Broadcast(InWidth, InHeight);
	}

	void CWindow::SetTitle(std::string_view NewTitle)
	{
		LK_ASSERT(GlfwWindow);
		LK_DEBUG_TAG("Window", "Set title: {}", NewTitle);
		glfwSetWindowTitle(GlfwWindow, NewTitle.data());
	}

	void CWindow::SetVSync(const bool Enabled)
	{
		LK_DEBUG_TAG("Window", "VSync: {}", Enabled ? "Enabled" : "Disabled");
		glfwSwapInterval(Enabled);
		Data.bVSync = Enabled;
	}

	uint16_t CWindow::GetRefreshRate() const
	{
		GLFWmonitor* Monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* Mode = glfwGetVideoMode(Monitor);
		LK_ASSERT(Monitor && Mode);
		return Mode->refreshRate;
	}

	void CWindow::Maximize()
	{
		if (GlfwWindow && !IsMaximized()) {
			glfwMaximizeWindow(GlfwWindow);
		}
	}

	void CWindow::Restore()
	{
		if (GlfwWindow && IsMaximized()) {
			glfwRestoreWindow(GlfwWindow);
		}
	}

	bool CWindow::IsMaximized() const
	{
		return (glfwGetWindowAttrib(GlfwWindow, GLFW_MAXIMIZED) == GLFW_TRUE);
	}

	void CWindow::SetIcon(const std::filesystem::path ImagePath)
	{
		LK_ASSERT(std::filesystem::exists(ImagePath), "Invalid path: {}", ImagePath);
		GLFWimage Icon{};
		Icon.pixels = stbi_load(ImagePath.generic_string().c_str(), &Icon.width, &Icon.height, 0, 4);
		LK_ASSERT(Icon.pixels);
		glfwSetWindowIcon(GlfwWindow, 1, &Icon);
		stbi_image_free(Icon.pixels);
	}

	void CWindow::Centralize()
	{
		GLFWmonitor* PrimaryMonitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* VideoMode = glfwGetVideoMode(PrimaryMonitor);
		LK_ASSERT(VideoMode);

		int WindowWidth{};
		int WindowHeight{};
		glfwGetWindowSize(GlfwWindow, &WindowWidth, &WindowHeight);

		int MonitorX{};
		int MonitorY{};
		glfwGetMonitorPos(PrimaryMonitor, &MonitorX, &MonitorY);

		const int PosX = MonitorX + (VideoMode->width - WindowWidth) / 2;
		const int PosY = MonitorY + (VideoMode->height - WindowHeight) / 2;

		glfwSetWindowPos(GlfwWindow, PosX, PosY);
		LK_TRACE_TAG("Window", "Centered at ({}, {})", PosX, PosY);
	}

}
