#pragma once

#include "core/core.h"
#include "core/delegate.h"
#include "core/layer.h"

namespace platformer2d {

	struct FKeyData;

	class CHudLayer : public CLayer
	{
	public:
		CHudLayer(std::string_view InLayerName = "HUD");
		CHudLayer(CHudLayer&&) = delete;
		CHudLayer(const CHudLayer&) = delete;
		~CHudLayer() = default;

		CHudLayer& operator=(CHudLayer&&) = delete;
		CHudLayer& operator=(const CHudLayer&) = delete;

		void Tick(float DeltaTime) override;
		void RenderUI() override;

		void OnAttach() override;
		void OnDetach() override;

		[[nodiscard]] bool IsInputDebugEnabled() const { return bInputDebug; }
		void SetInputDebug(bool Visible) { bInputDebug = Visible; }

	private:
		void OnKey(const FKeyData& Data);

		bool bInputDebug = false;
		Core::FDelegateHandle OnKeyHandle{};
	};
}
