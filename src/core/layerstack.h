#pragma once

#include "core.h"
#include "layer.h"

namespace platformer2d {

	class CLayerStack
	{
	public:
		CLayerStack() = default;
		~CLayerStack();

		void Destroy();

		bool PushLayer(std::shared_ptr<CLayer> Layer);
		bool PushOverlay(std::shared_ptr<CLayer> Overlay);
		bool PopLayer(std::shared_ptr<CLayer> Layer);
		bool PopOverlay(std::shared_ptr<CLayer> Layer);

		std::size_t Count() const { return Layers.size(); }

		std::shared_ptr<CLayer> operator[](const std::size_t Idx)
		{
			LK_ASSERT((Idx >= 0) && (Idx < Layers.size()));
			return Layers[Idx];
		}

		std::shared_ptr<CLayer> operator[](const std::size_t Idx) const
		{
			LK_ASSERT((Idx >= 0) && (Idx < Layers.size()));
			return Layers.at(Idx);
		}

		std::vector<std::shared_ptr<CLayer>>::iterator begin() { return Layers.begin(); }
		std::vector<std::shared_ptr<CLayer>>::iterator end() { return Layers.end(); }

	private:
		std::vector<std::shared_ptr<CLayer>> Layers{};
		uint32_t InsertIndex = 0;
	};

}
