#include <stdio.h>
#include <filesystem>

#include <catch2/catch_session.hpp>
#include <catch2/catch_test_macros.hpp>
#include <imgui/imgui.h>
#include <glm/glm.hpp>
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

/* 24 entries */
static float VertexData[] = {
	/* Vertices */
     0.0f,    0.50f,  0.0f,  1.0f,
     0.55f, -0.566f,  0.0f,  1.0f,
    -0.55f, -0.566f,  0.0f,  1.0f,

	/* Color */
     1.0f,     0.0f,  0.0f,  1.0f,
     0.0f,     1.0f,  0.0f,  1.0f,
     0.0f,     0.0f,  1.0f,  1.0f,
};

#define USE_RAW_SHADER_IMPL 0

TEST_CASE("Suite name", "[defines]")
{
	REQUIRE(LK_TEST_STRINGIFY(LK_TEST_SUITE) != "LK_TEST_SUITE");
}

TEST_CASE("Test name", "[defines]")
{
#ifdef LK_TEST_NAME
	REQUIRE(strcmp(LK_TEST_NAME, "LK_TEST_NAME") != 0);
#else
	FAIL("LK_TEST_NAME undefined");
#endif
}

int main(int argc, char* argv[])
{
	spdlog::info("Test: {}", LK_TEST_NAME);
	const int Result = Catch::Session().run(argc, argv);

	CTest Test;
	const std::filesystem::path& BinaryDir = Test.GetBinaryDirectory();
	const CWindow& Window = Test.GetWindow();
	const FWindowData& WindowData = Window.GetData();

	GLuint VertexBuffer;
	glGenBuffers(1, &VertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VertexData), VertexData, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	const std::filesystem::path VertexShaderPath = BinaryDir / "vertex.shader";
	const std::filesystem::path FragmentShaderPath = BinaryDir / "frag.shader";
	LK_VERIFY(std::filesystem::exists(VertexShaderPath), "Vertex shader does not exist");
	LK_VERIFY(std::filesystem::exists(FragmentShaderPath), "Fragment shader does not exist");
#if USE_RAW_SHADER_IMPL
	const uint32_t ProgramID = LoadShaders(VertexShaderPath, FragmentShaderPath);
#else
	CShader Shader(VertexShaderPath, FragmentShaderPath);
#endif

	static_assert(sizeof(VertexData) > 0);
	constexpr int ColorStartIdx = ((sizeof(VertexData) / sizeof(decltype(VertexData[0]))) / 2) - 1;

	glm::vec4 ClearColor{ 0.0f };
	glm::vec3 FragColor = { 
		VertexData[ColorStartIdx],
		VertexData[ColorStartIdx + (4 * 1)], 
		VertexData[ColorStartIdx + (4 * 2)]
	};

	const int FragShaderBytes = (3 * 4 * sizeof(float));

	while (true)
	{
		glfwPollEvents();
		LK_OpenGL_Verify(glClearColor(ClearColor.r, ClearColor.g, ClearColor.b, ClearColor.a));
		LK_OpenGL_Verify(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
		CTest::ImGui_NewFrame();

		ImGui::Text("%s", LK_TEST_STRINGIFY(LK_TEST_SUITE));
		ImGui::Text("Window size: (%d, %d)", WindowData.Width, WindowData.Height);
		ImGui::SliderFloat3("Background", &ClearColor.x, 0.0f, 1.0f, "%.2f");
		const bool UpdateFragShader = ImGui::SliderFloat3("Fragment Shader", &FragColor.x, 0.0f, 1.0f, "%.3f");

		glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, (void*)FragShaderBytes);

		if (UpdateFragShader)
		{
			VertexData[12] = FragColor.r;
			VertexData[13] = FragColor.g;
			VertexData[14] = FragColor.b;
			VertexData[15] = 1.0f;

			VertexData[16] = FragColor.r;
			VertexData[17] = FragColor.g;
			VertexData[18] = FragColor.b;
			VertexData[19] = 1.0f;

			VertexData[20] = FragColor.r;
			VertexData[21] = FragColor.g;
			VertexData[22] = FragColor.b;
			VertexData[23] = 1.0f;

			glBufferSubData(GL_ARRAY_BUFFER, (3 * 4 * sizeof(float)), (3 * 4 * sizeof(float)), &VertexData[12]);
		}

#if USE_RAW_SHADER_IMPL
		glUseProgram(ProgramID);
#else
		Shader.Bind();
#endif
		
		/* Draw triangle. */
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);

		CTest::ImGui_EndFrame();
		glfwSwapBuffers(Window.GetGlfwWindow());
		glfwPollEvents();
	}

	glfwTerminate();

	return 0;
}