#pragma once

#include "core/core.h"
#include "core/layer.h"
#include "renderer/imguilayer.h"

namespace platformer2d {

	class CUILayer : public CLayer
	{
	public:
		CUILayer(std::string_view InLayerName = "UI");
		~CUILayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void Tick(float DeltaTime) override;
		virtual void RenderUI() override;

		void BeginFrame();
		void EndFrame();

	private:
		std::unique_ptr<CImGuiLayer> ImGuiLayer = nullptr;
	};

}
