#pragma once

#include "core/layer.h"
#include "game/instance.h"
#include "renderer/texture.h"
#include "physics/events.h"
#include "scene/scene.h"

namespace platformer2d {

	class CRuntimeLayer : public CGameInstance
	{
	public:
		CRuntimeLayer();
		~CRuntimeLayer();

		void RenderUI() override;

		bool Serialize(const std::filesystem::path& OutFile) const override;
		bool Deserialize(const std::filesystem::path& InFile) override;

	private:
		void UI_ViewportTexture();
	};

}
