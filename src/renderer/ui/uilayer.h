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

		void Initialize();

		void OnAttach() override;
		void OnDetach() override;

		void Tick(float DeltaTime) override;
		void RenderUI() override;

		void BeginFrame();
		void EndFrame();

		enum class EMenu : std::uint8_t
		{
			None,
			MainMenu,
			LevelLauncher,
			Settings,
			Credits,
			COUNT
		};
		LK_ENUM(EMenu);

		void SetActiveMenu(EMenu InMenu);

		static void RequestMenu(EMenu InMenu);
		[[nodiscard]] static EMenu GetActiveMenu();

	private:
		void UI_MainMenu();
		void UI_Settings();
		void UI_Credits();

		void OnKey(const FKeyData& KeyData);

	private:
		std::unique_ptr<CImGuiLayer> ImGuiLayer = nullptr;

		static inline EMenu ActiveMenu = EMenu::MainMenu;
		static inline EMenu NextMenu = EMenu::None;

		struct
		{
			Core::FDelegateHandle OnKey;
		} DelegateHandles;
	};
}

