#pragma once

#include <stdint.h>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "core/log.h"
#include "core/delegate.h"

namespace platformer2d {

	struct FWindowSpecification
	{
		uint16_t Width = SCREEN_WIDTH;
		uint16_t Height = SCREEN_HEIGHT;
		std::string Title = "platformer2d";
		bool bStartMaximized = true;
		bool bVSync = true;
	};

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
		LK_DECLARE_EVENT(FOnFramebufferResized, CWindow, uint32_t, uint32_t);
	public:
		CWindow(const FWindowSpecification& InSpec);
		CWindow() = delete;
		~CWindow() = default;

		static CWindow* Get();

		void Initialize();
		void Destroy();

		void BeginFrame();
		void EndFrame();

		inline uint16_t GetWidth() const { return Data.Width; }
		inline uint16_t GetHeight() const { return Data.Height; }
		inline glm::vec2 GetSize() const { return { Data.Width, Data.Height }; }
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
		static inline FOnFramebufferResized OnFramebufferResized;
	private:
		const FWindowSpecification Spec;
		GLFWwindow* GlfwWindow = nullptr;
		FWindowData Data{};

		static inline CWindow* Instance = nullptr;
	};

}