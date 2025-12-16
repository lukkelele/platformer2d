#include "ui_core.h"

#include <queue>

#include "editor_resources.h"

namespace platformer2d {
	FEditorResources EditorResources;
}

namespace platformer2d::UI {

	namespace {
		uint32_t Counter = 0;
		int UIContextID = 0;
		std::array<char, 16 + 2 + 1> IDBuffer = { "##" };
		std::array<char, 1024 + 1> LabelIDBuffer;

		bool bDockspaceInitialized = false;
	}

	void PushID()
    {
        ImGui::PushID(UIContextID++);
        Counter = 0;
    }

    void PopID()
    {
        ImGui::PopID();
        UIContextID--;
    }

	const char* GenerateID()
	{
		std::snprintf(IDBuffer.data() + 2, 16, "%u", Counter++);
		return IDBuffer.data();
	}

	bool Begin(const char* WindowTitle, bool* Open, ImGuiWindowFlags WindowFlags)
    {
        UI::PushID();
        ImGui::Begin(WindowTitle, Open, WindowFlags);

		if (ImGuiWindow* ThisWindow = ImGui::GetCurrentWindow(); ThisWindow != nullptr) {
			if (ThisWindow->SkipItems) {
				ImGui::End();
				UI::PopID();
				return false;
			}
		}

		return true;
    }

    void End()
    {
        ImGui::End();
        UI::PopID();
    }

	void BeginViewport(CWindow* Window)
	{
		if (UI::DockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode) {
		    UI::CoreViewportFlags |= ImGuiWindowFlags_NoBackground;
			UI::HostWindowFlags |= ImGuiWindowFlags_NoBackground;
		}

        ImGuiViewport* Viewport = ImGui::GetMainViewport();

		FScopedStyle WindowRounding(ImGuiStyleVar_WindowRounding, 0.0f);
		FScopedStyle WindowBorderSize(ImGuiStyleVar_WindowBorderSize, 0.0f);
		FScopedStyle WindowPadding(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		FScopedColor MenuBarBg(ImGuiCol_MenuBarBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::SetNextWindowPos(Viewport->Pos);
        ImGui::SetNextWindowSize(Viewport->Size);
        ImGui::SetNextWindowViewport(Viewport->ID);
        ImGui::Begin(PanelID::CoreViewport, nullptr, UI::HostWindowFlags);
		ImGuiID DockspaceID = ImGui::GetID(PanelID::Dockspace);

		/**
		 * The sidebar nodes do not get the same sizes even though the size ratios
		 * for the dock splitting is the same. A 0.04 increment in the right sidebar
		 * is needed to make the sizes the same.
		 */
		if (ImGui::DockBuilderGetNode(DockspaceID) == nullptr) {
			LK_INFO("Creating dockspace layout");

			/* Remove existing layout. */
			LK_DEBUG("Removing existing docking layout (ID: {})", DockspaceID);
			ImGui::DockBuilderRemoveNode(DockspaceID);

			const uint32_t WindowWidth = Window->GetWidth();
			const uint32_t WindowHeight = Window->GetHeight();

			float LeftSidebarFraction = 0.25f;
			float RightSidebarFraction = 0.25f;
			float TopBarFraction = 0.05f;

			/* Wide monitor. */
			if ((WindowWidth > 1920) || (WindowHeight > 1080)) {
				LeftSidebarFraction = 0.22f;
				RightSidebarFraction = 0.18f;
				TopBarFraction = 0.05f;
			/* Normal 16:9 monitor. */
			} else if ((WindowWidth <= 1920) || (WindowHeight <= 1080)) {
				LeftSidebarFraction = 0.28f;
				RightSidebarFraction = 0.26f;
				TopBarFraction = 0.05f;
			}

			/* Add empty node. */
			ImGuiDockNodeFlags DockFlags = ImGuiDockNodeFlags_DockSpace 
				| ImGuiDockNodeFlags_NoWindowMenuButton;
			ImGui::DockBuilderAddNode(DockspaceID, DockFlags);
			ImGui::DockBuilderSetNodeSize(DockspaceID, Viewport->Size);

			ImGuiID DockID_Main = DockspaceID;
			ImGuiID DockID_Left = ImGui::DockBuilderSplitNode(DockID_Main, ImGuiDir_Left, LeftSidebarFraction, nullptr, &DockID_Main);
			ImGuiID DockID_Left_Top = ImGui::DockBuilderSplitNode(DockID_Left, ImGuiDir_Up, 0.34f, nullptr, &DockID_Left);

			ImGuiID DockID_Right = ImGui::DockBuilderSplitNode(DockID_Main, ImGuiDir_Right, RightSidebarFraction, nullptr, &DockID_Main);
			ImGuiID DockID_Right_Top = ImGui::DockBuilderSplitNode(DockID_Right, ImGuiDir_Up, 0.52f, nullptr, &DockID_Right);

			ImGuiID DockID_Bottom = ImGui::DockBuilderSplitNode(DockID_Main, ImGuiDir_Down, 0.32f, nullptr, &DockID_Main);
			ImGuiID DockID_Bottom_Right = ImGui::DockBuilderSplitNode(DockID_Bottom, ImGuiDir_Right, 0.42f, nullptr, &DockID_Bottom);

			ImGuiID DockID_Top = ImGui::DockBuilderSplitNode(DockID_Main, ImGuiDir_Up, TopBarFraction, nullptr, &DockID_Main);

			ImGui::DockBuilderDockWindow(PanelID::EditorViewport, DockID_Main);
			ImGui::DockBuilderDockWindow(PanelID::Sidebar1, DockID_Left_Top);
			ImGui::DockBuilderDockWindow(PanelID::Sidebar2, DockID_Right_Top);
			ImGui::DockBuilderDockWindow(PanelID::Topbar, DockID_Top);

			ImGui::DockBuilderDockWindow(PanelID::ContentBrowser, DockID_Bottom);
			ImGui::DockBuilderDockWindow(PanelID::Selection, DockID_Left);
			ImGui::DockBuilderDockWindow(PanelID::SceneManager, DockID_Right);

			/* Finish the dockspace. */
			ImGui::DockBuilderFinish(DockspaceID);

			/* Disable splitting over entire viewport. */
			if (ImGuiDockNode* DockNode = ImGui::DockBuilderGetNode(ImGui::GetID(PanelID::Dockspace))) {
				DockNode->LocalFlags |= ImGuiDockNodeFlags_NoDockingOverMe;
				DockNode->LocalFlags |= ImGuiDockNodeFlags_NoDockingSplit;
			}

			ImGuiDockNode* Sidebar1Node = ImGui::DockBuilderGetNode(DockID_Left);
			ImGuiDockNode* Sidebar2Node = ImGui::DockBuilderGetNode(DockID_Right_Top);
			LK_VERIFY(Sidebar1Node);
			LK_VERIFY(Sidebar2Node);

			bDockspaceInitialized = true;
		}

		if (!bDockspaceInitialized) {
			LK_DEBUG("Initializing dockspace layout");
			ImGuiDockNode* DockspaceNode = ImGui::DockBuilderGetNode(DockspaceID);
			LK_VERIFY(DockspaceNode, "Dockspace node is nullptr");
			DockspaceNode->LocalFlags |= ImGuiDockNodeFlags_NoDockingOverMe;
			DockspaceNode->LocalFlags |= ImGuiDockNodeFlags_NoDockingSplit;

			/* Disable docking in the central node. */
			ImGuiDockNode* CentralNode = FindCentralNode(ImGui::GetID(PanelID::Dockspace));
			LK_VERIFY(CentralNode, "Cannot find central node");
			CentralNode->LocalFlags |= ImGuiDockNodeFlags_NoDocking;

			bDockspaceInitialized = true;
		}

		/* Submit the dockspace. */
		ImGui::DockSpace(DockspaceID, ImVec2(0, 0), UI::DockspaceFlags);
		ImGui::End(); /* PanelID::CoreViewport */

		ImGui::SetNextWindowPos(Viewport->Pos);
		ImGui::SetNextWindowSize(Viewport->Size);
		ImGui::SetNextWindowViewport(Viewport->ID);
		ImGui::Begin(PanelID::CoreViewport, nullptr, UI::CoreViewportFlags);
	}

	ImGuiDockNode* FindCentralNode(const ImGuiID DockspaceID)
	{
		ImGuiDockNode* RootNode = ImGui::DockBuilderGetNode(DockspaceID);
		if (!RootNode) {
			return nullptr;
		}

		std::queue<ImGuiDockNode*> NodeQueue;
		NodeQueue.push(RootNode);

		while (!NodeQueue.empty()) {
			ImGuiDockNode* CurrentNode = NodeQueue.front();
			NodeQueue.pop();

			if (CurrentNode->IsCentralNode()) {
				return CurrentNode;
			}

			/* Add child nodes to the queue to check recursively. */
			if (CurrentNode->ChildNodes[0]) {
				NodeQueue.push(CurrentNode->ChildNodes[0]);
			}

			if (CurrentNode->ChildNodes[1]) {
				NodeQueue.push(CurrentNode->ChildNodes[1]);
			}
		}

		return nullptr;
	}

}
