#include "selectionpanel.h"

#include "core/window.h"
#include "core/selectioncontext.h"
#include "core/input/keyboard.h"
#include "game/instance.h"
#include "renderer/color.h"
#include "renderer/font.h"
#include "renderer/renderer.h"
#include "ui_core.h"
#include "widgets.h"
#include "scene/scene.h"

namespace platformer2d::UI {

	void SelectionPanel()
	{
		if (!CGameInstance::IsValid()) {
			return;
		}

		auto& GameInstance = CGameInstance::Get();
		std::shared_ptr<CScene> Scene = CGameInstance::Get().GetScene();
		if (!Scene) {
			return;
		}

		if (!UI::Begin(UI::PanelID::Selection, nullptr)) {
			return;
		}

		const LUUID SelectedID = CSelectionContext::GetSelected();
		if (std::shared_ptr<CActor> Actor = Scene->GetActor(SelectedID); Actor != nullptr) {
			UI::Actor::Data(Actor);
			UI::Actor::DeleteButton(Actor, Scene);
		} else {
			ImGui::Dummy(ImVec2(0, 4));
		}

		UI::End(); /* ~SelectionPanel */
	}

}
