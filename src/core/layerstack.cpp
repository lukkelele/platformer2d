#include "layerstack.h"

namespace platformer2d {

	CLayerStack::~CLayerStack()
	{
		if (!Layers.empty())
		{
			Destroy();
		}
	}

	void CLayerStack::Destroy()
	{
		LK_DEBUG_TAG("LayerStack", "Releasing {} layers", Layers.size());

		/* Pop overlays first. */
		for (auto Iter = Layers.begin() + InsertIndex; Iter != Layers.end();)
		{
			LK_TRACE_TAG("LayerStack", "Popping overlay \"{}\", {} overlays left", (*Iter)->GetName(), Layers.size() - 1);
			(*Iter)->OnDetach();
			Iter = Layers.erase(Iter);
		}

		/* Pop the rest of the layers. */
		for (auto Iter = Layers.begin(); Iter != Layers.end();)
		{
			LK_TRACE_TAG("LayerStack", "Popping layer \"{}\", {} layers left", (*Iter)->GetName(), Layers.size() - 1);
			(*Iter)->OnDetach();
			Iter = Layers.erase(Iter);
			InsertIndex--;
		}
	}

	bool CLayerStack::PushLayer(std::shared_ptr<CLayer> Layer)
	{
		if (auto Iter = Layers.emplace((Layers.begin() + InsertIndex), Layer); Iter != Layers.end())
		{
			InsertIndex++;
			(*Iter)->OnAttach();
			return true;
		}

		return false;
	}

	bool CLayerStack::PushOverlay(std::shared_ptr<CLayer> Overlay)
	{
		if (Layers.emplace_back(Overlay))
		{
			Overlay->OnAttach();
			return true;
		}

		return false;
	}

	bool CLayerStack::PopLayer(std::shared_ptr<CLayer> Layer)
	{
		auto Iter = std::find(Layers.begin(), Layers.begin() + InsertIndex, Layer);
		if (Iter != (Layers.begin() + InsertIndex))
		{
			Layer->OnDetach();

			Layers.erase(Iter);
			InsertIndex--;
			return true;
		}

		return false;
	}

	bool CLayerStack::PopOverlay(std::shared_ptr<CLayer> Overlay)
	{
		auto Iter = std::find(Layers.begin() + InsertIndex, Layers.end(), Overlay);
		if (Iter != Layers.end())
		{
			Overlay->OnDetach();
			Layers.erase(Iter);
			return true;
		}

		return false;
	}

}
