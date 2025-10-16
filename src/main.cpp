#include <stdio.h>
#include <filesystem>

#include <imgui/imgui.h>
#include <stb/stb_image.h>
#include <spdlog/spdlog.h>

#include "core/assert.h"
#include "core/window.h"
#include "renderer/renderer.h"
#include "renderer/texture.h"

int main(int argc, char* argv[])
{
	using namespace platformer2d;
	std::printf("platformer2d\n");

	platformer2d::CWindow Window(800, 600);
	Window.Initialize();
	CRenderer::Initialize();

	const FWindowData& WindowData = Window.GetData();

	const char* TexturePath = "../../assets/textures/bricks.jpg";
	if (std::filesystem::exists(TexturePath)) 
	{
		spdlog::info("Texture exists: {}", TexturePath);
	} 
	else 
	{
		spdlog::error("Texture does not exist: {}", TexturePath);
	}

	stbi_uc* TextureData = nullptr;
	int ReadWidth, ReadHeight, ReadChannels;
	TextureData = (uint8_t*)stbi_loadf(TexturePath, &ReadWidth, &ReadHeight, &ReadChannels, 4);
	spdlog::info("width={} height={}", ReadWidth, ReadHeight);
	LK_ASSERT(TextureData && (ReadWidth > 0) && (ReadHeight > 0), "Corrupt texture");
	CTexture Texture(ReadWidth, ReadHeight, TextureData);

	while (true)
	{
		Window.BeginFrame();

		ImGui::Text("platformer2d");
		ImGui::Text("Window size: (%d, %d)", WindowData.Width, WindowData.Height);

		ImGui::Begin("Widgets");
		static int IntSlider = 0;
		ImGui::SliderInt("Integer Slider", &IntSlider, 0, 10, "%d");
		ImGui::End(); /* Widgets */

		Window.EndFrame();
	}

	Window.Destroy();

	return 0;
}