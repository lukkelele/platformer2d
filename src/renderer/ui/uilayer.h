#pragma once

#include "core/core.h"
#include "core/layer.h"
#include "core/input/keyboard.h"
#include "imguilayer.h"

namespace platformer2d {

	class CUILayer : public CLayer
	{
	public:
		CUILayer(std::string_view InLayerName = "UI");
		~CUILayer() = default;

		virtual void OnAttach() override;
		virtual void OnDetach() override;

		virtual void Tick(float DeltaTime) override;
		virtual void RenderUI() override;

		void BeginFrame();
		void EndFrame();

		enum class EMenu
		{
			None,
			MainMenu,
			LevelLauncher,
		};

		void SetActiveMenu(EMenu InMenu);

	private:
		void UI_MainMenu();
		void UI_GameMenu();

		void OnKeyPressed(const FKeyData& KeyData);

	private:
		std::unique_ptr<CImGuiLayer> ImGuiLayer = nullptr;
		EMenu ActiveMenu = EMenu::MainMenu;

		struct
		{
			Core::FDelegateHandle OnKeyPressed;
		} DelegateHandles;
	};

	namespace Enum {
		inline const char* ToString(const CUILayer::EMenu Menu)
		{
			const char* S = "";
		#define _(EnumValue) case CUILayer::EMenu::EnumValue: S = #EnumValue; break
			switch (Menu) {
				_(None);
				_(MainMenu);
				_(LevelLauncher);
				default:
					LK_THROW_ENUM_ERR(Menu);
					break;
			}
		#undef _
			return S;
		}
	}

}
