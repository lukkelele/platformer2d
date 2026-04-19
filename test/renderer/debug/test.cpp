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

	static const std::string TestName = LK_TEST_NAME;
	constexpr bool NO_TEST_INIT = false;

	static glm::vec3 P1 = {0.30f, -0.40, 0.50f};
	static glm::vec3 ROT = {0.0f, 0.0f, 0.0f};
	static float RADIUS = 0.05f;
	static bool bDrawCircle = true;
	static bool bDrawCircleFilled = true;
	static bool bDrawLine = true;
	static bool bDrawRectangle = false;

	static void RenderViewportTexture();

	CTest::CTest(const int Argc, char* Argv[])
		: CTestBase(Argc, Argv, NO_TEST_INIT)
	{
		lklog::init(lklog::level::debug);

		const FWindowSpecification WindowSpec = {
			.Width = SCREEN_WIDTH,
			.Height = SCREEN_HEIGHT,
			.Title = LK_TEST_NAME,
			.bStartMaximized = true,
			.bVSync = true};
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
		std::shared_ptr<CFramebuffer> Framebuffer = CRenderer::GetViewportFramebuffer();
		Framebuffer->Bind();

		/* Create actors. */
		std::vector<std::shared_ptr<CActor>> Actors;
		constexpr std::size_t N = 1;
		for (std::size_t Idx = 0; Idx < N; Idx++) {
			FActorSpecification Spec;
			Spec.Name = lklog::format("Actor-{}", Idx);
			Spec.Pos = {0.30f, 0.0f, 0.0f}; /* @fixme: Does not do anything, only BodySpec.Position is working */
			Spec.OutlineColor = FColor::Magenta;
			Spec.OutlineThickness = 8.0f;

			FBodySpecification BodySpec;
			FPolygon Polygon = {
				.Size = {0.40f, 0.20f},
				.Radius = 0.50f,
				.Rotation = 0.0f,
			};
			BodySpec.Shape.emplace<FPolygon>(Polygon);
			BodySpec.Position = {
				0.30f,
				0.05f,
			};

			Actors.emplace_back(std::make_shared<CActor>(Spec, BodySpec));
		}

		LK_INFO("Actors: {}", Actors.size());

		GLFWwindow* GlfwWindow = Window->GetGlfwWindow();
		while (!glfwWindowShouldClose(GlfwWindow)) {
			Framebuffer->Clear();
			Window->BeginFrame();

			CKeyboard::Update();
			CRenderer::BeginFrame();
			CRenderer::StartBatch();

			if (bDrawCircle) {
				CRenderer::DrawCircle(P1, ROT, RADIUS, FColor::Red);
			}
			if (bDrawCircleFilled) {
				CRenderer::DrawCircleFilled(P1, RADIUS, FColor::Red, 5.0f);
			}
			if (bDrawLine) {
				CRenderer::DrawLine(glm::vec3(0.0f, 0.0f, 1.0f), P1, FColor::Black, 6);
			}
			if (bDrawRectangle) {
				CRenderer::DrawQuad({0.30f, 0.25f}, {0.14f, 0.10f}, FColor::Green);
			}

			for (const auto& Actor : Actors) {
				const FTransformComponent& TC = Actor->GetTransformComponent();
				CRenderer::DrawQuad(
					Actor->GetPosition(),
					TC.Scale,
					Actor->GetTexture(),
					Actor->GetColor(),
					glm::degrees(TC.GetRotation2D()),
					Actor->IsOutlineEnabled() ? Actor->GetOutlineThickness() : 0.0f,
					Actor->GetOutlineColor());
			}

			CRenderer::EndFrame();

			ImGui_NewFrame();
			RenderViewportTexture();

			ImGui::SetNextWindowFocus();
			ImGui::Begin(TestName.c_str());
			{
				ImGui::Checkbox("Draw Circle", &bDrawCircle);
				ImGui::SameLine();
				ImGui::Checkbox("Draw Circle Filled", &bDrawCircleFilled);
				ImGui::SameLine();
				ImGui::Checkbox("Draw Line", &bDrawLine);
				ImGui::SameLine();
				ImGui::Checkbox("Draw Smaller Rectangle", &bDrawRectangle);

				ImGui::BeginChild("##P0", ImVec2(ImGui::GetWindowSize().x * 0.30f, 40.0f));
				UI::Widget::DragFloat3("P0", P1, 0.0f, 0.010f);
				ImGui::EndChild();

				ImGui::BeginChild("##Radius", ImVec2(ImGui::GetWindowSize().x * 0.30f, 40.0f));
				UI::Widget::DragFloat("Radius", RADIUS, 0.010f, 0.0f, 10.0f);
				ImGui::EndChild();

				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
				ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0);
				ImGui::BeginChild("##Actors", ImVec2(ImGui::GetWindowSize().x * 0.30f, 800.0f));
				UI::HeaderTextCentralized("Actors", EFont::Roboto, EFontModifier::Bold);
				for (const auto& Actor : Actors) {
					UI::Widget::ActorNode::Data(Actor);
					UI::Widget::DrawComponents(Actor);
				}
				ImGui::PopStyleVar(2);
				ImGui::EndChild();
			}
			ImGui::End();

			ImGui_EndFrame();

			Window->EndFrame();
			CKeyboard::TransitionPressedKeys();
		}
	}

	void CTest::Destroy()
	{
		LK_DEBUG_TAG("Test", "Destroy: {}", LK_TEST_NAME);
		CRenderer::Destroy();

		Window->Destroy();
		Window.reset();
	}

	static void RenderViewportTexture()
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

}
