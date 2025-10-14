#pragma once

#include <stdint.h>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "core/log.h"

namespace platformer2d {

	struct FWindowData
	{
		uint16_t Width = 0;
		uint16_t Height = 0;
		std::string Title{};
		bool bVSync = false;

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
		CWindow(uint16_t InWidth, uint16_t InHeight, std::string_view InTitle = "platformer2d");
		CWindow() = delete;
		~CWindow() = default;

		static CWindow* Get();

		void Initialize();
		void Destroy();

		void BeginFrame();
		void EndFrame();

		void SetVSync(bool Enabled);
		bool GetVSync() const { return Data.bVSync; }

		const FWindowData& GetData() const { return Data; }
		GLFWwindow* GetGlfwWindow() const { return GlfwWindow; }

	private:
		GLFWwindow* GlfwWindow = nullptr;
		FWindowData Data{};
	};

}