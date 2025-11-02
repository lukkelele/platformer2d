#pragma once

#include "core/layer.h"
#include "game/gameinstance.h"
#include "renderer/texture.h"

namespace platformer2d::Level {

	class CTestLevel : public IGameInstance
	{
	public:
		CTestLevel();
		virtual ~CTestLevel() = default;

		virtual void Initialize() override;
		virtual void Destroy() override;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void Tick(float DeltaTime) override;
		virtual CCamera* GetActiveCamera() const override;
		virtual void RenderUI() override;

	private:
		void CreatePlayer();
		void CreatePlatform();
		void CreateTerrain();

		void UI_Level();
		void UI_Player();

	private:
		std::unique_ptr<CPlayer> Player = nullptr;
		std::shared_ptr<CActor> Platform = nullptr;
	};

}
