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

	static void AddVertexBufferToVertexArray(const GLuint VAO, const GLuint VBO, const FVertexBufferLayout& Layout);

	static void ComputePositionOffset(glm::vec2& Offset, const float LoopDuration = 5.0f)
	{
		LK_ASSERT(LoopDuration > 0.0f);
		const float Scale = (3.14159f * 2.0f) / LoopDuration;
		const float ElapsedTime = static_cast<float>(glfwGetTime());
		const float TimeThroughLoop = std::fmodf(ElapsedTime, LoopDuration);

		Offset.x = std::cosf(TimeThroughLoop * Scale) * 0.50f;
		Offset.y = std::sinf(TimeThroughLoop * Scale) * 0.50f;
	}

	static constexpr float VertexData[] = {
		   /* Vertices */
		 0.0f,   0.25f,  0.0f,
		 0.25f, -0.25f,  0.0f,
		-0.25f, -0.25f,  0.0f,
	};

	CTest::CTest(const int Argc, char* Argv[])
		: CTestBase(Argc, Argv)
	{
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

		/* 2) Add vertex buffer. */
		static_assert(sizeof(VertexData) > 0);
		GLuint VertexBuffer;
		glGenBuffers(1, &VertexBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(VertexData), VertexData, GL_STATIC_DRAW);

		/* 3) Buffer layout, based from the layouts in the vertex shader. */
		const FVertexBufferLayout BufferLayout = {
			{ "pos", EShaderDataType::Float3, },
		};

		/* 4) Configure the vertex array based on the buffer layout. */
		AddVertexBufferToVertexArray(VertexArray, VertexBuffer, BufferLayout);

		/* 5) Vertex array object ready. */
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

			/* Background */
			ImGui::SliderFloat3("Background", &ClearColor.x, 0.0f, 1.0f, "%.2f");
			const bool UpdateFragShader = ImGui::SliderFloat4("Fragment Shader", &FragColor.x, 0.0f, 1.0f, "%.3f");
			/* Offset */
			static glm::vec2 Offset{ 0.0f, 0.0f };
			ImGui::SliderFloat2("Position Offset", &Offset.x, -0.50f, 0.50f, "%.2f");
			/* Loop Duration */
			static float LoopDuration = 5.0f;
			ImGui::SliderFloat("Loop Duration", &LoopDuration, 0.10f, 10.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
			const bool bLoopDuration = ImGui::IsItemActive();
			ImGui::SameLine(0.0f, 4.0f);
			ImGui::Text("Held: %s", bLoopDuration ? "Yes" : "No");

			glBindVertexArray(VertexArray);
			glUseProgram(ProgramID);

			if (!bLoopDuration) 
			{
				/* Do not offset whenever loop duration slider is held. */
				ComputePositionOffset(Offset, LoopDuration);
				const GLint OffsetLoc = glGetUniformLocation(ProgramID, "u_offset");
				glUniform2f(OffsetLoc, Offset.x, Offset.y);
			}

			const GLint ColorLoc = glGetUniformLocation(ProgramID, "u_color");
			glUniform4f(ColorLoc, FragColor.r, FragColor.g, FragColor.b, FragColor.a);
			
			/* Draw triangle. */
			glDrawArrays(GL_TRIANGLES, 0, 3);

			glUseProgram(0);

			CTest::ImGui_EndFrame();
			glfwSwapBuffers(Window.GetGlfwWindow());
			glfwPollEvents();
		}
	}

	void CTest::Destroy()
	{
		glfwTerminate();
	}

	static void AddVertexBufferToVertexArray(const GLuint VAO, const GLuint VBO, const FVertexBufferLayout& Layout)
	{
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		int VertexBufferIndex = 0;
		for (const FVertexBufferElement& Element : Layout)
		{
			LK_INFO("VertexBufferIndex={} Size={} ComponentCount={}", VertexBufferIndex, Element.Size, Element.GetComponentCount());
			switch (Element.Type)
			{
				case EShaderDataType::Float:
				case EShaderDataType::Float2:
				case EShaderDataType::Float3:
				case EShaderDataType::Float4:
				{
					glEnableVertexAttribArray(VertexBufferIndex);
					glVertexAttribPointer(VertexBufferIndex,
										  Element.GetComponentCount(),
										  OpenGL::ShaderDataTypeToOpenGLBaseType(Element.Type),
										  (Element.Normalized ? GL_TRUE : GL_FALSE),
										  Layout.GetStride(),
										  (const void*)Element.Offset);
					VertexBufferIndex++;
					break;
				}
				case EShaderDataType::Int:
				case EShaderDataType::Int2:
				case EShaderDataType::Int3:
				case EShaderDataType::Int4:
				case EShaderDataType::Bool:
				{
					glEnableVertexAttribArray(VertexBufferIndex);
					glVertexAttribIPointer(VertexBufferIndex,
										   Element.GetComponentCount(),
										   OpenGL::ShaderDataTypeToOpenGLBaseType(Element.Type),
										   Layout.GetStride(),
										   (const void*)Element.Offset);
					VertexBufferIndex++;
					break;
				}
				case EShaderDataType::Mat3:
				case EShaderDataType::Mat4:
				{
					uint8_t Count = Element.GetComponentCount();
					for (uint8_t Idx = 0; Idx < Count; Idx++)
					{
						glEnableVertexAttribArray(VertexBufferIndex);
						glVertexAttribPointer(VertexBufferIndex,
											  Count, 
											  OpenGL::ShaderDataTypeToOpenGLBaseType(Element.Type), 
											  (Element.Normalized ? GL_TRUE : GL_FALSE),
											  Layout.GetStride(),
											  (const void*)(Element.Offset + sizeof(float) * Count * Idx));
						glVertexAttribDivisor(VertexBufferIndex, 1);
						VertexBufferIndex++;
					}
					break;
				}

				default: 
					LK_ERROR("Unhandled shader type: {}", Enum::ToString(Element.Type));
			}
		}
	}

}