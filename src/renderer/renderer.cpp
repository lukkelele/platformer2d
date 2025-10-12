#include "renderer.h"

#include <array>
#include <atomic>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_common.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/norm.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/backends/imgui_impl_opengl3.h>
#include <spdlog/spdlog.h>

#include "core/window.h"
#include "backendinfo.h"
#include "imguilayer.h"
#include "opengl.h"
#include "rendercommandqueue.h"

namespace platformer2d {

	struct FRendererData
	{
		struct FQuad 
		{
			GLuint VAO = 0;
			GLuint VBO = 0;
			GLuint EBO = 0;
		} Quad;
	};

	namespace 
	{
		FRendererData Data{};

		std::unordered_map<std::string, std::shared_ptr<CShader>> ShaderLibrary{};
		const std::string SHADER_QUAD = "Quad";

		std::array<CRenderCommandQueue*, 2> CommandQueue;
		std::atomic<uint32_t> CommandQueueSubmissionIndex = 0;

		OpenGL::FBackendInfo BackendInfo{};

		constexpr float QuadVertices[] = {
		  /*  Position    Texture Coordinates */
			-1.0f, -1.0f,    0.0f, 0.0f,
			 1.0f, -1.0f,    1.0f, 0.0f,
			 1.0f,  1.0f,    1.0f, 1.0f,
			-1.0f,  1.0f,    0.0f, 1.0f
		};

		constexpr uint32_t QuadIndices[] = { 
			0, 1, 2,  
			2, 3, 0 
		};
	}

	void CRenderer::Initialize()
	{
		const GLenum GladInitResult = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		LK_OpenGL_Verify(glEnable(GL_LINE_SMOOTH));
		LK_OpenGL_Verify(glEnable(GL_BLEND));
		LK_OpenGL_Verify(glEnable(GL_DEPTH_TEST));
		OpenGL::LoadInfo(BackendInfo);
		LK_INFO("OpenGL {}.{}", BackendInfo.Version.Major, BackendInfo.Version.Minor);

		LK_DEBUG("Creating {} render command queues", CommandQueue.size());
		for (int Idx = 0; Idx < CommandQueue.size(); Idx++)
		{
			CommandQueue[Idx] = new CRenderCommandQueue();
		}

		const FVertexBufferLayout QuadLayout = {
			{ "pos",      EShaderDataType::Float2, },
			{ "texcoord", EShaderDataType::Float2, },
		};

		LK_DEBUG("Creating quad VAO, VBO and EBO");
		Data.Quad.VAO = OpenGL::VertexArray::Create();
		Data.Quad.VBO = OpenGL::VertexBuffer::Create(QuadVertices, QuadLayout);
		Data.Quad.EBO = OpenGL::ElementBuffer::Create(QuadIndices);
		Data.Quad.EBO = OpenGL::ElementBuffer::Create(QuadIndices);
		LK_VERIFY((Data.Quad.VAO != 0) && (Data.Quad.VBO != 0) && (Data.Quad.EBO != 0));

		ImGuiLayer = std::make_unique<CImGuiLayer>(CWindow::Get()->GetGlfwWindow());

		LK_INFO("Loading shaders from: {}", SHADERS_DIR);
		/* Shader: Quad */
		{
			auto [Iter, Inserted] = ShaderLibrary.try_emplace(SHADER_QUAD, std::make_shared<CShader>(SHADERS_DIR "/player.shader"));
			LK_VERIFY(Inserted == true);
			std::shared_ptr<CShader>& Shader = Iter->second;
			Shader->Set("u_texture", 0);
		}

#ifdef LK_BUILD_DEBUG
		for (const auto& [ShaderName, Shader] : ShaderLibrary)
		{
			LK_INFO("Shader: {} ({})", Shader->GetFilepath().filename(), ShaderName);
		}
#endif
	}

	void CRenderer::BeginFrame()
	{
		LK_OpenGL_Verify(glClearColor(ClearColor.r, ClearColor.g, ClearColor.b, ClearColor.a));
		LK_OpenGL_Verify(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
	
		ImGuiLayer->BeginFrame();
	}

	void CRenderer::EndFrame()
	{
		ImGuiLayer->EndFrame();
	}

	void CRenderer::DrawQuad(const glm::vec2& Pos, const glm::vec2& Size, 
							 const glm::vec4& Color, const float RotationDeg)
	{
		glm::mat4 Transform = glm::mat4(1.0f);
		Transform = glm::translate(Transform, glm::vec3(Pos, 0.0f));

		Transform = glm::translate(Transform, glm::vec3(Size.x * 0.50f, Size.y * 0.50f, 0.0f));
		Transform = glm::rotate(Transform, glm::radians(RotationDeg), glm::vec3(0.0f, 0.0f, 1.0f)); 
		Transform = glm::translate(Transform, glm::vec3(Size.x * -0.50f, Size.y * -0.50f, 0.0f));

		Transform = glm::scale(Transform, glm::vec3(Size, 1.0f));

		ShaderLibrary[SHADER_QUAD]->Set("u_transform", Transform);
		ShaderLibrary[SHADER_QUAD]->Set("u_color", Color);

		LK_OpenGL_Verify(glBindVertexArray(Data.Quad.VAO));
		LK_OpenGL_Verify(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0));
	}

}