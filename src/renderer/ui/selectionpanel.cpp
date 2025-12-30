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
		CGameInstance* GameInstance = CGameInstance::Get();
		if (!GameInstance) {
			return;
		}

		std::shared_ptr<CScene> Scene = GameInstance->GetScene();
		if (!Scene) {
			return;
		}

		if (!UI::Begin(PanelID::Selection, nullptr)) {
			return;
		}

		const LUUID SelectedID = CSelectionContext::GetSelected();
		if (std::shared_ptr<CActor> Actor = Scene->FindActor(SelectedID); Actor != nullptr) {
			UI::Widget::ActorNode::Data(Actor);
			UI::Widget::DrawComponents(Actor);
			UI::Widget::ActorNode::DeleteButton(Actor, Scene);
		} else {
			ImGui::Dummy(ImVec2(0, 4));
		}

		UI::End(); /* ~SelectionPanel */
	}

}
