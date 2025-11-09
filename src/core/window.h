#pragma once

#include <stdint.h>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "core/log.h"
#include "core/delegate.h"

namespace platformer2d {

	struct FWindowData
	{
		uint16_t Width = 0;
		uint16_t Height = 0;
		std::string Title{};
		bool bVSync = false;
		class CWindow* WindowRef = nullptr;

		FWindowData() = default;
		FWindowData(const uint16_t InWidth, const uint16_t InHeight, std::string_view InTitle)
			: Width(InWidth)
			, Height(InHeight)
			, Title(InTitle)
		{
		}
	};

	class CWindow
	{
	public:
		LK_DECLARE_EVENT(FOnResized, CWindow, uint16_t /* Width */, uint16_t /* Height */);
	public:
		CWindow(uint16_t InWidth, uint16_t InHeight, std::string_view InTitle = "platformer2d");
		CWindow() = delete;
		~CWindow() = default;

		static CWindow* Get();

		void Initialize();
		void Destroy();

		void BeginFrame();
		void EndFrame();

		void SetSize(uint16_t InWidth, uint16_t InHeight);
		void SetTitle(std::string_view NewTitle);
		void SetVSync(bool Enabled);
		bool GetVSync() const { return Data.bVSync; }
		uint16_t GetRefreshRate() const;
		void Maximize();
		bool IsMaximized() const;

		const FWindowData& GetData() const { return Data; }
		inline GLFWwindow* GetGlfwWindow() const { return GlfwWindow; }

	private:
		void SetIcon(std::filesystem::path ImagePath);
		void Centralize();

	public:
		static inline FOnResized OnResized;
	private:
		GLFWwindow* GlfwWindow = nullptr;
		FWindowData Data{};

		static inline CWindow* Instance = nullptr;
	};

}