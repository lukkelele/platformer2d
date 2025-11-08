#include "window.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <stb/stb_image.h>
#include <spdlog/spdlog.h>

#include "renderer/opengl.h"
#include "input/keyboard.h"
#include "input/mouse.h"

namespace platformer2d {

	CWindow::CWindow(const uint16_t InWidth, const uint16_t InHeight, std::string_view InTitle)
		: Data(InWidth, InHeight, InTitle)
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
			LK_ERROR("GLFW error ({}): {}", Error, Description);
		});

		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, LK_OPENGL_MAJOR);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, LK_OPENGL_MINOR);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef LK_BUILD_DEBUG
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif

		LK_DEBUG_TAG("Window", "Create ({}, {})", Data.Width, Data.Height);
		Data.WindowRef = this;
		GlfwWindow = glfwCreateWindow(Data.Width, Data.Height, Data.Title.c_str(), nullptr, nullptr);
		LK_VERIFY(GlfwWindow);
		glfwMakeContextCurrent(GlfwWindow);
		glfwSetWindowUserPointer(GlfwWindow, &Data);

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

		SetVSync(true);

		glfwSetKeyCallback(GlfwWindow, [](GLFWwindow* Window, int Key, int ScanCode, int Action, int Modifiers)
		{
			LK_TRACE_TAG("Window", "Key={} Action={} Modifiers={}", Key, Action, Modifiers);
			FWindowData& WindowDataRef = *((FWindowData*)glfwGetWindowUserPointer(Window));
			switch (Action)
			{
				case GLFW_PRESS:
				{
					CKeyboard::UpdateKeyState(static_cast<EKey>(Key), EKeyState::Pressed);
					break;
				}
				case GLFW_RELEASE:
				{
					CKeyboard::UpdateKeyState(static_cast<EKey>(Key), EKeyState::Released);
					break;
				}
				case GLFW_REPEAT:
				{
					CKeyboard::UpdateKeyState(static_cast<EKey>(Key), EKeyState::Held);
					break;
				}
			}
		});

		glfwSetMouseButtonCallback(GlfwWindow, [](GLFWwindow* Window, int Button, int Action, int Modifiers)
		{
			switch (Action)
			{
				case GLFW_PRESS:
				{
					CMouse::UpdateButtonState(static_cast<EMouseButton>(Button), EMouseButtonState::Pressed);
					break;
				}
				case GLFW_RELEASE:
				{
					CMouse::UpdateButtonState(static_cast<EMouseButton>(Button), EMouseButtonState::Released);
					break;
				}
			}
		});

		glfwSetScrollCallback(GlfwWindow, [](GLFWwindow* Window, double OffsetX, double OffsetY)
		{
			if (OffsetY > 0)
			{
				CMouse::UpdateScrollState(EMouseScrollDirection::Up);
			}
			else if (OffsetY < 0)
			{
				CMouse::UpdateScrollState(EMouseScrollDirection::Down);
			}
		});

		/* Set window icon. */
		const std::filesystem::path IconPath = TEXTURES_DIR "/test/test_player.png";
		SetIcon(IconPath);

		Centralize();
	}

	void CWindow::Destroy()
	{
		glfwTerminate();
		GlfwWindow = nullptr;
	}

	void CWindow::BeginFrame()
	{
		glfwPollEvents();
	}

	void CWindow::EndFrame()
	{
		glfwSwapBuffers(GlfwWindow);
		glfwPollEvents();
	}

	void CWindow::SetSize(const uint16_t InWidth, const uint16_t InHeight)
	{
		if ((Data.Width != InWidth) || (Data.Height != InHeight))
		{
			Data.Width = InWidth;
			Data.Height = InHeight;
			OnResized.Broadcast(InWidth, InHeight);
		}
	}

	void CWindow::SetTitle(std::string_view NewTitle)
	{
		LK_ASSERT(GlfwWindow);
		LK_DEBUG_TAG("Window", "Set title: {}", NewTitle);
		glfwSetWindowTitle(GlfwWindow, NewTitle.data());
	}

	void CWindow::SetVSync(const bool Enabled)
	{
		LK_DEBUG_TAG("Window", "VSync: {}", Enabled ? "enabled" : "disabled");
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
