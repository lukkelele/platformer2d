#pragma once

#include <stdint.h>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace platformer2d {

	struct FWindowData
	{
		uint16_t Width = 0;
		uint16_t Height = 0;
		std::string Title{};

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
		CWindow(uint16_t InWidth, uint16_t InHeight, std::string_view InTitle = "");
		CWindow() = delete;
		~CWindow() = default;

		static CWindow* Get();

		void Initialize();
		void Destroy();

		void BeginFrame();
		void EndFrame();

		const FWindowData& GetData() const { return Data; }

	public:
		GLFWwindow* GlfwWindow = NULL; /** @todo: Make private */
	private:
		FWindowData Data{};
	};

}