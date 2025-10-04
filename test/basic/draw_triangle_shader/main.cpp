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
#include "renderer/texture.h"
#include "renderer/shader.h"

#include "test.h"

#include "shader.h"

#ifndef LK_TEST_SUITE
#error "LK_TEST_SUITE missing"
#endif

using namespace platformer2d;
using namespace platformer2d::test;

static const GLfloat Vertices_Triangle[] = 
{
   -1.0f, -1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    0.0f,  1.0f, 0.0f,
};

#define USE_RAW_SHADER_IMPL 0

int main(int Argc, char* Argv[])
{
	CTest Test(Argc, Argv);
	const std::filesystem::path& BinaryDir = Test.GetBinaryDirectory();
	const CWindow& Window = Test.GetWindow();
	const FWindowData& WindowData = Window.GetData();

	GLuint VertexBuffer;
	glGenBuffers(1, &VertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
	/* Submit the vertices to OpenGL. */
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices_Triangle), Vertices_Triangle, GL_STATIC_DRAW);

	const std::filesystem::path VertexShaderPath = BinaryDir / "vertex.shader";
	const std::filesystem::path FragmentShaderPath = BinaryDir / "frag.shader";
	LK_VERIFY(std::filesystem::exists(VertexShaderPath), "Vertex shader does not exist");
	LK_VERIFY(std::filesystem::exists(FragmentShaderPath), "Fragment shader does not exist");
#if USE_RAW_SHADER_IMPL
	const uint32_t ProgramID = LoadShaders(VertexShaderPath, FragmentShaderPath);
#else
	CShader Shader(VertexShaderPath, FragmentShaderPath);
#endif

	while (true)
	{
		glfwPollEvents();
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		CTestBase::ImGui_NewFrame();

		ImGui::Text("%s", LK_TEST_NAME);
		ImGui::Text("Resolution: %dx%d", WindowData.Width, WindowData.Height);

		glEnableVertexAttribArray(0); /* Position attribute 0 in the vertex shader. */
		glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
		glVertexAttribPointer(
			0,        /* Match layout in shader */
		    3,        /* Size */
		    GL_FLOAT, /* Type */
		    GL_FALSE, /* Normalized */
		    0,        /* Stride */
		    (void*)0  /* Array buffer offset */
		);

		/* Use the shader. */
#if USE_RAW_SHADER_IMPL
		glUseProgram(ProgramID);
#else
		Shader.Bind();
#endif

		/* Draw triangle. */
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glDisableVertexAttribArray(0);

		CTestBase::ImGui_EndFrame();
		glfwSwapBuffers(Window.GetGlfwWindow());
		glfwPollEvents();
	}

	glfwTerminate();

	return 0;
}