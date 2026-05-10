#pragma once

#include <cstdint>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "core/log.h"
#include "core/delegate.h"

namespace platformer2d {

	struct FWindowSpecification
	{
		std::uint16_t Width = SCREEN_WIDTH;
		std::uint16_t Height = SCREEN_HEIGHT;
		std::string Title = "platformer2d";
		bool bStartMaximized = true;
		bool bVSync = true;
	};

	struct FWindowData
	{
		std::uint16_t Width = 0;
		std::uint16_t Height = 0;
		std::string Title{};
		bool bVSync = false;
		class CWindow* WindowRef = nullptr;
	};

	class CWindow
	{
	public:
		LK_DECLARE_EVENT(FOnResized, CWindow, uint16_t /* Width */, uint16_t /* Height */);
		LK_DECLARE_EVENT(FOnFramebufferResized, CWindow, uint32_t, uint32_t);

	private:
		CWindow() = default;

	public:
		~CWindow() = default;
		CWindow(CWindow&&) = delete;
		CWindow(const CWindow&) = delete;

		CWindow& operator=(CWindow&&) = delete;
		CWindow& operator=(const CWindow&) = delete;

		static CWindow& Get();

		void Initialize(const FWindowSpecification& Spec);
		void Destroy();

		void BeginFrame();
		void EndFrame();

		[[nodiscard]] std::uint16_t GetWidth() const { return Data.Width; }
		[[nodiscard]] std::uint16_t GetHeight() const { return Data.Height; }
		[[nodiscard]] glm::vec2 GetSize() const { return {Data.Width, Data.Height}; }
		void SetSize(std::uint16_t InWidth, std::uint16_t InHeight);
		void SetTitle(std::string_view NewTitle);
		void SetVSync(bool Enabled);
		[[nodiscard]] bool GetVSync() const { return Data.bVSync; }
		[[nodiscard]] std::uint16_t GetRefreshRate() const;
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
		GLFWwindow* GlfwWindow = nullptr;
		FWindowData Data{};
	};

}
