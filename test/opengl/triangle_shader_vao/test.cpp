#include "test.h"

#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <stb/stb_image.h>

#include "core/assert.h"
#include "core/window.h"
#include "renderer/texture.h"
#include "renderer/shader.h"

namespace platformer2d::test {

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
		const int Result = Catch::Session().run(Args.Argc, Args.Argv);
		const std::filesystem::path& BinaryDir = GetBinaryDirectory();
		const CWindow& Window = GetWindow();
		const FWindowData& WindowData = Window.GetData();

		/* 1) Create vertex array. */
		GLuint VertexArray;
		glGenVertexArrays(1, &VertexArray);
		glBindVertexArray(VertexArray);

		/* 2) Add vertex buffer. */
		static_assert(sizeof(VertexData) > 0);
		GLuint VertexBuffer;
		glGenBuffers(1, &VertexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(VertexData), VertexData, GL_STATIC_DRAW);

		/* 3) Configure vertex attributes. */
		const int FragShaderBytes = (3 * 4 * sizeof(float));
		const GLsizei Stride = 0;
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, Stride, 0);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, Stride, (void*)FragShaderBytes);

		/* 4) Vertex array object ready. */
		glBindVertexArray(0);

		const std::filesystem::path VertexShaderPath = BinaryDir / "vertex.shader";
		const std::filesystem::path FragmentShaderPath = BinaryDir / "frag.shader";
		CShader Shader(VertexShaderPath, FragmentShaderPath);

		glm::vec4 ClearColor{ 0.0f };
		glm::vec4 FragColor{ 1.0f, 0.481f, 0.508f, 1.0f };

		const GLuint ProgramID = Shader.GetProgramID();
		LK_INFO("Shader program: {}", ProgramID);

		while (true)
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
			
			/* Draw triangle. */
			glDrawArrays(GL_TRIANGLES, 0, 3);

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