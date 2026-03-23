#pragma once

#include "core/core.h"

namespace platformer2d {

	class CLayer
	{
	public:
		CLayer(std::string_view InName);
		CLayer() = delete;
		virtual ~CLayer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}

		virtual void Tick(float DeltaTime) = 0;
		virtual void RenderUI() {}

		void SetLayerName(std::string_view InName) { LayerName = InName; }
		std::string_view GetLayerName() const { return LayerName; }

	protected:
		std::string LayerName;
	};

}
