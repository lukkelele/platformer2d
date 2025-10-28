#pragma once

#include "core/layer.h"
#include "game/gameinstance.h"

namespace platformer2d::Level {

	class CTestLevel : public IGameInstance
	{
	public:
		CTestLevel();
		virtual ~CTestLevel();

		virtual void Initialize() override;
		virtual void Destroy() override;

		virtual void Tick(float DeltaTime) override;
		virtual void RenderUI() override;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

	private:
		void UI_Player();
		void UI_Physics();
	};

}
