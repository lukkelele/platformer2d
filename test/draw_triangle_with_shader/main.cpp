#include <stdio.h>
#include <filesystem>

#include <imgui/imgui.h>
#include <spdlog/spdlog.h>
#include <stb/stb_image.h>

#include "core/assert.h"
#include "core/window.h"
#include "renderer/texture.h"

#include "test.h"

#include "shader.h"

#ifndef LK_TEST_SUITE
#error "LK_TEST_SUITE missing"
#endif

static const GLfloat Vertices_Triangle[] = 
{
   -1.0f, -1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    0.0f,  1.0f, 0.0f,
};

int main(int argc, char* argv[])
{
	using namespace platformer2d;
	spdlog::info("Running: {}", LK_TEST_SUITE);

	const std::filesystem::path BinaryDir = test::GetBinaryDirectory();
	spdlog::info("Binary dir: {}", BinaryDir.generic_string());

	platformer2d::CWindow Window(800, 600);
	Window.Initialize();
	const FWindowData& WindowData = Window.GetData();

	GLuint VertexBuffer;
	glGenBuffers(1, &VertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
	/* Submit the vertices to OpenGL. */
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices_Triangle), Vertices_Triangle, GL_STATIC_DRAW);

	const std::filesystem::path VertexShaderPath = BinaryDir / "vertex.shader";
	const std::filesystem::path FragmentShaderPath = BinaryDir / "frag.shader";
	//const uint32_t ProgramID = LoadShaders(VertexShaderPath.generic_string().c_str(), FragmentShaderPath.generic_string().c_str());
	const uint32_t ProgramID = LoadShaders(VertexShaderPath, FragmentShaderPath);

	while (true)
	{
		Window.BeginFrame();

		ImGui::Text("platformer2d");
		ImGui::Text("Window size: (%d, %d)", WindowData.Width, WindowData.Height);

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

		/* Use the shader. */
		glUseProgram(ProgramID);
		
		/* Draw triangle. */
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glDisableVertexAttribArray(0);

		Window.EndFrame();
	}

	Window.Destroy();

	return 0;
}