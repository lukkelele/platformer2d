#include "test.h"

#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>
#include <stb/stb_image.h>

#include "core/application.h"
#include "core/assert.h"
#include "core/window.h"
#include "core/input/keyboard.h"
#include "physics/physicsworld.h"
#include "renderer/opengl.h"
#include "renderer/renderer.h"
#include "renderer/ui/ui.h"
#include "renderer/ui/widgets.h"

namespace platformer2d::test {

	namespace {
		constexpr bool NO_TEST_INIT = false;
	}

	CTest::CTest(const int Argc, char* Argv[])
		: CTestBase(Argc, Argv, NO_TEST_INIT)
	{
		CLog::Initialize();

		const FWindowSpecification WindowSpec = {
			.Width = SCREEN_WIDTH,
			.Height = SCREEN_HEIGHT,
			.Title = LK_TEST_NAME,
			.bStartMaximized = true,
			.bVSync = true
		};
		Window = std::make_unique<CWindow>(WindowSpec);
		Window->Initialize();
		InitRenderContext(Window->GetGlfwWindow());

		CPhysicsWorld::Initialize();
		CRenderer::Initialize();
		CKeyboard::Initialize();
		CMouse::Initialize();
	}

	void CTest::Run()
	{
		bRunning = true;
#ifdef RUN_CATCH_TESTS
		const int CatchResult = Catch::Session().run(Args.Argc, Args.Argv);
#endif

		CWindow* Window = CWindow::Get();
		const FWindowData& WindowData = Window->GetData();

		std::shared_ptr<CFramebuffer> Framebuffer = CRenderer::GetViewportFramebuffer();
		Framebuffer->Bind();

		bool bDrawCircle = true;
		bool bDrawCircleFilled = true;
		bool bDrawLine = true;

		auto RenderViewportTexture = [&]()
		{
			ImGuiViewport* Viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(Viewport->Pos);
			ImGui::SetNextWindowSize(Viewport->Size);
			ImGui::SetNextWindowViewport(Viewport->ID);
			ImGui::Begin(UI::PanelID::CoreViewport, nullptr, UI::CoreViewportFlags);

			const ImVec2 WindowSize = ImGui::GetWindowSize();

			std::shared_ptr<CFramebuffer> Framebuffer = CRenderer::GetViewportFramebuffer();
			std::shared_ptr<CTexture> ViewportTexture = Framebuffer->GetImage(0);

			ImGui::Image(
				(ImTextureID)ViewportTexture->GetID(),
				WindowSize,
				ImVec2(0, 1),       /* UV0 */
				ImVec2(1, 0),       /* UV1 */
				ImVec4(1, 1, 1, 1), /* Tint Color   */
				ImVec4(1, 1, 1, 0)  /* Border Color */
			);

			ImGui::End();
		};

		std::string TestName = LK_TEST_NAME;

		while (bRunning)
		{
			Framebuffer->Clear();
			Window->BeginFrame();

			CKeyboard::Update();
			CRenderer::BeginFrame();
			CRenderer::StartBatch();

			static glm::vec3 P1 = { 0.30f, -0.40, 0.50f };
			static glm::vec3 ROT = { 0.0f, 0.0f, 0.0f };
			static float RADIUS = 0.05f;
			if (bDrawCircle)
			{
				CRenderer::DrawCircle(P1, ROT, RADIUS, FColor::Red);
			}
			if (bDrawCircleFilled)
			{
				CRenderer::DrawCircleFilled(P1, RADIUS, FColor::Red, 5.0f);
			}
			if (bDrawLine)
			{
				CRenderer::DrawLine(glm::vec3(0.0f, 0.0f, 1.0f), P1, FColor::Black, 6);
			}

			CRenderer::DrawQuad({ -0.25f, 0.10f }, { 0.10f, 0.10f }, FColor::Green);

			CRenderer::EndFrame();

			ImGui_NewFrame();
			RenderViewportTexture();

			ImGui::SetNextWindowFocus();
			ImGui::Begin(TestName.c_str());

			ImGui::Checkbox("Draw Circle", &bDrawCircle);
			ImGui::SameLine();
			ImGui::Checkbox("Draw Circle Filled", &bDrawCircleFilled);
			ImGui::SameLine();
			ImGui::Checkbox("Draw Line", &bDrawLine);

			UI::Widget::Vec3Control("P0", P1, 0.0f, 0.010f);
			UI::Widget::DragFloat("Radius", RADIUS, 0.010f, 0.0f, 10.0f);

			ImGui::End();
			ImGui_EndFrame();

			Window->EndFrame();
			CKeyboard::TransitionPressedKeys();
		}
	}

	void CTest::Destroy()
	{
		LK_DEBUG_TAG("Test", "Destroy");
	}

}