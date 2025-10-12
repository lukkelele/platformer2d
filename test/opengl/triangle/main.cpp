#include <stdio.h>
#include <filesystem>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <spdlog/spdlog.h>
#include <stb/stb_image.h>

#include "core/assert.h"
#include "core/window.h"
#include "renderer/opengl.h"

#include "test_base.h"

#ifndef LK_TEST_SUITE
#error "LK_TEST_SUITE missing"
#endif

using namespace platformer2d;
using namespace platformer2d::test;

namespace {
	const GLfloat Vertices_Triangle[] = 
	{
	   -1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		0.0f,  1.0f, 0.0f,
	};

	constexpr ImGuiWindowFlags CoreViewportFlags = ImGuiWindowFlags_NoTitleBar
		| ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize
		| ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar
		| ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus
		| ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDocking
		| ImGuiWindowFlags_NoBackground;
}

int main(int argc, char* argv[])
{
	spdlog::info("Running: {}", LK_TEST_STRINGIFY(LK_TEST_SUITE));

	platformer2d::CWindow Window(800, 600, LK_TEST_STRINGIFY(LK_TEST_SUITE));
	Window.Initialize();
	CTestBase::InitRenderContext(Window.GetGlfwWindow());
	const FWindowData& WindowData = Window.GetData();

	GLuint VertexBuffer;
	glGenBuffers(1, &VertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
	/* Submit the vertices to OpenGL. */
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices_Triangle), Vertices_Triangle, GL_STATIC_DRAW);

	while (true)
	{
		glfwPollEvents();
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
		ImGuiViewport* Viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(Viewport->Pos);
        ImGui::SetNextWindowSize(Viewport->Size);
        ImGui::SetNextWindowViewport(Viewport->ID);
		ImGui::Begin(LK_TEST_STRINGIFY(LK_TEST_SUITE), NULL, CoreViewportFlags);

		ImGui::Text("%s", LK_TEST_NAME);
		ImGui::Text("Resolution: %dx%d", WindowData.Width, WindowData.Height);

		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
		glVertexAttribPointer(
			0,        /* Match layout in shader */
		    3,        /* Size */
		    GL_FLOAT, /* Type */
		    GL_FALSE, /* Normalized */
		    0,        /* Stride */
		    (void*)0  /* Array buffer offset */
		);

		glDrawArrays(GL_TRIANGLES, 0, 3);
		glDisableVertexAttribArray(0);

		ImGui::End(); /* LK_TEST_SUITE */
		ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(Window.GetGlfwWindow());
	}

	Window.Destroy();

	return 0;
}