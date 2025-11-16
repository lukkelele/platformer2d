#include "uilayer.h"

#include "core/window.h"

#include "renderer/renderer.h"
#include "renderer/ui/ui.h"
#include "renderer/ui/widgets.h"

namespace platformer2d {

	CUILayer::CUILayer(std::string_view InName)
		: CLayer(InName)
	{
		ImGuiLayer = std::make_unique<CImGuiLayer>(CWindow::Get()->GetGlfwWindow());
	}

	void CUILayer::OnAttach()
	{
		LK_DEBUG_TAG("UILayer", "Initializing UI subsystem");
		UI::Initialize();
	}

	void CUILayer::OnDetach()
	{
		LK_TRACE_TAG("UILayer", "OnDetach");
		if (ImGuiLayer)
		{
			LK_DEBUG_TAG("UILayer", "Destroy ImGui layer");
			ImGuiLayer->Destroy();
			ImGuiLayer.reset();
		}
	}

	void CUILayer::Tick(const float DeltaTime)
	{
		LK_UNUSED(DeltaTime);

		/* Draw dark overlay whenever the pause menu is open. */
		if (UI::IsGameMenuOpen())
		{
			if (CWindow* Window = CWindow::Get(); Window != nullptr)
			{
				const glm::vec2 WindowSize = Window->GetSize();
				static constexpr glm::vec4 OverlayColor = { 0.10f, 0.10f, 0.10f, 0.90f };
				CRenderer::DrawQuad(glm::vec3(0.0f, 0.0f, 0.0f), WindowSize, OverlayColor);
			}
		}
	}

	void CUILayer::RenderUI()
	{
#if 0
		if (UI::IsGameMenuOpen())
		{
			if (CWindow* Window = CWindow::Get(); Window != nullptr)
			{
				const glm::vec2 WindowSize = CWindow::Get()->GetSize();
				static constexpr glm::vec4 OverlayColor = { 0.10f, 0.10f, 0.10f, 0.90f };
				CRenderer::DrawQuad(glm::vec3(0.0f, 0.0f, 0.60f), WindowSize, OverlayColor);
			}
		}
#endif
	}

	void CUILayer::BeginFrame()
	{
		ImGuiLayer->BeginFrame();
	}

	void CUILayer::EndFrame()
	{
		UI::Render();
		ImGuiLayer->EndFrame();
	}

}
