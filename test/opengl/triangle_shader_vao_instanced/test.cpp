#include "test.h"

#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <stb/stb_image.h>

#include "core/assert.h"
#include "core/window.h"
#include "renderer/texture.h"
#include "renderer/shader.h"
#include "renderer/vertexbufferlayout.h"

namespace platformer2d::test {

	static constexpr float VertexPositions[] = {
		 0.0f,    0.25f,  0.0f,
		 0.25f,  -0.25f,  0.0f,
		-0.25f,  -0.25f,  0.0f,
	};

	static constexpr float VertexOffsets[] = {
		 -0.70, 0.0f, /* Left */
		  0.0f, 0.0f, /* Center */
		  0.70, 0.0f, /* Right */
	};

	CTest::CTest(const int Argc, char* Argv[])
		: CTestBase(Argc, Argv)
	{
		LK_ASSERT(Window && Window->GetGlfwWindow());
		InitRenderContext(Window->GetGlfwWindow());
		OpenGL::LoadInfo(BackendInfo);
		LK_INFO("OpenGL {}.{}", BackendInfo.Version.Major, BackendInfo.Version.Minor);
		LK_INFO("ImGui Version: {}", ImGui::GetVersion());
	}

	void CTest::Run()
	{
		Running = true;
		const int Result = Catch::Session().run(Args.Argc, Args.Argv);
		const std::filesystem::path& BinaryDir = GetBinaryDirectory();
		const CWindow& Window = GetWindow();
		const FWindowData& WindowData = Window.GetData();

		/* 1) Create vertex array. */
		GLuint VertexArray;
		glGenVertexArrays(1, &VertexArray);
		glBindVertexArray(VertexArray);

		/* 2) Add buffer for vertex positions. */
		GLuint VBO;
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(VertexPositions), VertexPositions, GL_STATIC_DRAW);

		/* 3) Configure layout 0 (position). */
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (3 * sizeof(float)), nullptr);

		/* 4) Add instance buffer for offsets. */
		GLuint InstanceVBO;
		glGenBuffers(1, &InstanceVBO);
		glBindBuffer(GL_ARRAY_BUFFER, InstanceVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(VertexOffsets), VertexOffsets, GL_STATIC_DRAW);

		/* 5) Configure layout 1 (offsets). */
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, (2 * sizeof(float)), nullptr);
		glVertexAttribDivisor(1, 1); /* Advance ONCE per instance. */

		/* 6) Vertex array object ready. */
		glBindVertexArray(0);

		const std::filesystem::path VertexShaderPath = BinaryDir / "vertex.shader";
		const std::filesystem::path FragmentShaderPath = BinaryDir / "frag.shader";
		CShader Shader(VertexShaderPath, FragmentShaderPath);

		glm::vec4 ClearColor{ 0.10f, 0.10f, 0.10f, 1.0f };
		glm::vec4 FragColor{ 0.410f, 0.181f, 0.813f, 1.0f };

		const GLuint ProgramID = Shader.GetProgramID();
		LK_DEBUG("Shader program: {}", ProgramID);

		while (Running)
		{
			glfwPollEvents();
			glClearColor(ClearColor.r, ClearColor.g, ClearColor.b, ClearColor.a);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			CTest::ImGui_NewFrame();

			ImGui::Text("%s", LK_TEST_NAME);
			ImGui::Text("Resolution: %dx%d", WindowData.Width, WindowData.Height);

			ImGui::SliderFloat3("Background", &ClearColor.x, 0.0f, 1.0f, "%.2f");
			const bool UpdateFragShader = ImGui::SliderFloat4("Fragment Shader", &FragColor.x, 0.0f, 1.0f, "%.3f");

			glBindVertexArray(VertexArray);

			const GLint UniformLoc = glGetUniformLocation(ProgramID, "u_color");
			glUseProgram(ProgramID);
			glUniform4f(UniformLoc, FragColor.r, FragColor.g, FragColor.b, FragColor.a);
			
			glDrawArraysInstanced(GL_TRIANGLES, 0, 3, 3/*=Instances*/);

			glUseProgram(0);

			CTest::ImGui_EndFrame();
			glfwSwapBuffers(Window.GetGlfwWindow());
		}
	}

	void CTest::Destroy()
	{
		glfwTerminate();
	}

}