#pragma once

#include "core/core.h"
#include "core/layer.h"

namespace platformer2d {

	class CHudLayer : public CLayer
	{
	public:
		CHudLayer(std::string_view InLayerName = "HUD");
		~CHudLayer() = default;

		void Tick(float DeltaTime) override;
		void RenderUI() override;

		void OnAttach() override;
		void OnDetach() override;
	};
}
