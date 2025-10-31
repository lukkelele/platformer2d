#include "ui.h"

#include "core/input/keyboard.h"
#include "renderer/color.h"
#include "renderer/font.h"

namespace platformer2d::UI {

	FOnGameMenuOpened OnGameMenuOpened;

	enum class EGameMenuView
	{
		Default,
		Settings,
	};

	struct FGameMenu
	{
		bool bOpen = false;

		EGameMenuView View = EGameMenuView::Default;
		EGameMenuView LastView = EGameMenuView::Default;

		struct FSettings
		{
			bool bDebug = false;
			bool bStyleEditor = false;
			bool bIDStackTool = false;
		} Settings;
	};

	namespace {
		FGameMenu GameMenu{};
	}

	bool IsGameMenuOpen()
	{
		return GameMenu.bOpen;
	}

	void RainbowTextGradient(const char* Text, const float Speed = 0.15f);
	void RainbowTextSynced(const char* Text, float WaveLengthPx = 180.0f, float SpeedPxPerSec = 30.0f,
						   float Saturation = 1.0f, float Value = 1.0f);

	FORCEINLINE static void UI_GameMenu_Default()
	{
		const ImVec2 StartCursorPos = ImGui::GetCursorPos();
		const ImVec2 MenuSize = ImGui::GetContentRegionAvail();
		ImGuiStyle& Style = ImGui::GetStyle();

		/* Menu title. */
		{
			ImGui::SetCursorPosY(16.0f);
			Font::Push(EFont::Roboto, EFontSize::Banner, EFontModifier::BoldItalic);
			static const std::string Title = "platformer2d";
			const ImVec2 TitleSize = ImGui::CalcTextSize(Title.c_str());
			ImGui::SetCursorPosX((MenuSize.x * 0.50f) - (TitleSize.x * 0.50f));
			RainbowTextSynced("platformer2d");
			Font::Pop();

			Font::Push(EFont::Roboto, EFontSize::Regular, EFontModifier::BoldItalic);
			static const std::string Desc = "a game written by Lukas Gunnarsson";
			const ImVec2 DescSize = ImGui::CalcTextSize(Desc.c_str());
			ImGui::SetCursorPosX((MenuSize.x * 0.50f) - (DescSize.x * 0.50f));
			ImGui::TextColored(ImColor(IM_COL32(100, 100, 100, 255)), Desc.c_str());
			Font::Pop();

			/* @todo Versioning info here */

			ImGui::Dummy(ImVec2(0.0f, 52.0f));
		}

		static constexpr float OptionPercentage = 0.80f;
		const ImVec2 ButtonSize = { MenuSize.x * OptionPercentage, 62.0f };
		ImGui::SetCursorPosX(((1.0f - OptionPercentage) * 0.50f) * MenuSize.x);

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 14.0f);
		Font::Push(EFont::Roboto, EFontSize::Larger, EFontModifier::Bold);
		if (ImGui::Button(LK_ICON_COG " Settings", ButtonSize))
		{
			GameMenu.View = EGameMenuView::Settings;
		}
		ImGui::PopStyleVar(1); /* FrameRounding */

		const ImVec2 Avail = ImGui::GetContentRegionAvail();
		ImGui::SetCursorPosY(Avail.y + Style.ItemSpacing.y);

		const ImVec2 HalfButtonSize = { (ButtonSize.x * 0.50f), ButtonSize.y };

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
		/* Quit button. */
		ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(255, 45, 45, 200));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(255, 45, 45, 90));
		ImGui::SetCursorPosX(((1.0f - OptionPercentage) * 0.50f) * MenuSize.x);
		if (ImGui::Button("Quit Game", HalfButtonSize))
		{
			Core::Global.bShouldShutdown = true;
		}
		ImGui::PopStyleColor(2);

		/* Play button. */
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0, 205, 15, 192));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0, 205, 15, 90));
		if (ImGui::Button(LK_ICON_PLAY " Play", HalfButtonSize))
		{
			CloseGameMenu();
		}
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar(1);

		Font::Pop();
	}

	FORCEINLINE static void UI_GameMenu_Settings()
	{
		FGameMenu::FSettings& Settings = GameMenu.Settings;
		ImGuiStyle& Style = ImGui::GetStyle();

		const ImVec2 StartCursorPos = ImGui::GetCursorPos();
		const ImVec2 MenuSize = ImGui::GetContentRegionAvail();
		const ImVec2 ButtonSize = { MenuSize.x, 62.0f };

		Font::Push(EFont::Roboto, EFontSize::Larger, EFontModifier::Bold);

		static constexpr float PaddingX = 12.0f;
		static constexpr float PaddingY = 12.0f;
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 4.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
		ImGui::SetCursorPos(ImVec2(PaddingX, PaddingY));
		ImGui::Checkbox("Debug", &GameMenu.Settings.bDebug);
		ImGui::PopStyleVar(2);

		ImGui::Dummy(ImVec2(0.0f, PaddingY * 0.50f));
		if (ImGui::Button("Style Editor", ButtonSize))
		{
			Settings.bStyleEditor = !Settings.bStyleEditor;
		}
		if (Settings.bStyleEditor)
		{
			ImGui::Begin("##StyleEditor", &Settings.bStyleEditor);
			ImGui::ShowStyleEditor(&Style);
			ImGui::End();
		}

		if (ImGui::Button("ID Tool", ButtonSize))
		{
			Settings.bIDStackTool = !Settings.bIDStackTool;
		}
		if (Settings.bIDStackTool)
		{
			ImGui::ShowIDStackToolWindow(&Settings.bIDStackTool);
		}

		Font::Pop();

		{
			const ImVec2 Avail = ImGui::GetContentRegionAvail();
			ImGui::SetCursorPosY(Avail.y + (2.0f * ButtonSize.y));
		}
		if (ImGui::Button(LK_ICON_BACKWARD, ButtonSize))
		{
			GameMenu.View = EGameMenuView::Default;
		}
	}

	FORCEINLINE static void UI_GameMenu()
	{
		ImGuiViewport* Viewport = ImGui::GetMainViewport();
		if (Viewport == nullptr)
		{
			return;
		}

		static constexpr float YFactor = 0.80f;
		const ImVec2 WindowSize((Viewport->Size.x * 0.33f), Viewport->Size.y * YFactor);
		const ImVec2 WindowPos(
			(Viewport->Size.x * 0.50f) - (WindowSize.x * 0.50f),
			(Viewport->Size.y * (1.0f - YFactor)) * 0.50f);

		ImGui::SetNextWindowPos(WindowPos, ImGuiCond_Always);
		ImGui::SetNextWindowSize(WindowSize, ImGuiCond_Always);
		static constexpr int WindowFlags = ImGuiWindowFlags_NoResize
			| ImGuiWindowFlags_NoDecoration
			| ImGuiWindowFlags_NoDocking
			| ImGuiWindowFlags_NoCollapse;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 0.0f));
		if (!ImGui::Begin("##GameMenu", nullptr, WindowFlags))
		{
			return;
		}

		ImGui::PopStyleVar(2);
		const ImVec2 StartCursorPos = ImGui::GetCursorPos();
		const ImVec2 MenuSize = ImGui::GetWindowSize();
		const ImVec2 ButtonSize = { MenuSize.x, 62.0f };

		switch (GameMenu.View)
		{
			case EGameMenuView::Default:
				UI_GameMenu_Default();
				break;
			case EGameMenuView::Settings:
				UI_GameMenu_Settings();
				break;
		}

		if (GameMenu.View != GameMenu.LastView)
		{
			LK_DEBUG_TAG("UI", "View changed");
		}

		GameMenu.LastView = GameMenu.View;

		ImGui::End();
	}

	static void OnKeyPressed(const FKeyData& KeyData)
	{
		if (KeyData.State == EKeyState::Pressed)
		{
			switch (KeyData.Key)
			{
				case EKey::Escape:
					ToggleGameMenu();
					break;
			}
		}
	}

	void Initialize()
	{
		static bool bInitialized = false;
		LK_VERIFY(bInitialized == false, "UI initialized multiple times");
		CKeyboard::OnKeyPressed.Add(OnKeyPressed);
		bInitialized = true;
	}

	void Render()
	{
		if (IsGameMenuOpen())
		{
			UI_GameMenu();
		}
	}

	void OpenGameMenu()
	{
		GameMenu.bOpen = true;
		OnGameMenuOpened.Broadcast(GameMenu.bOpen);
	}

	void CloseGameMenu()
	{
		GameMenu.bOpen = false;
		OnGameMenuOpened.Broadcast(GameMenu.bOpen);
	}

	void ToggleGameMenu()
	{
		GameMenu.bOpen = !GameMenu.bOpen;
		LK_DEBUG_TAG("UI", "Toggle Game Menu: {}", GameMenu.bOpen ? "Open" : "Closed");
		OnGameMenuOpened.Broadcast(GameMenu.bOpen);
	}

	void RainbowTextGradient(const char* Text, const float Speed)
	{
		const float Time = ImGui::GetTime() * 0.5f;

		const ImVec2 StartPos = ImGui::GetCursorScreenPos();
		ImFont* Font = ImGui::GetFont();
		const float FontSize = ImGui::GetFontSize();

		ImVec2 Pos = StartPos;
		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		for (const char* Ptr = Text; *Ptr; Ptr++)
		{
			float Hue = std::fmodf(Time + (*Ptr) * Speed, 1.0f);
			ImVec4 Col;
			ImGui::ColorConvertHSVtoRGB(Hue, 1.0f, 1.0f, Col.x, Col.y, Col.z);
			Col.w = 1.0f;

			const char Character[2] = {*Ptr, 0};
			DrawList->AddText(Font, FontSize, Pos, ImGui::ColorConvertFloat4ToU32(Col), Character);

			Pos.x += Font->CalcTextSizeA(FontSize, FLT_MAX, 0.0f, Character).x;
		}
		ImGui::Dummy(ImVec2(Pos.x - StartPos.x, FontSize));
	}

	void RainbowTextSynced(const char* Text, const float WaveLengthPx,
						   const float SpeedPxPerSec, const float Saturation, const float Value)
	{
		LK_ASSERT(Text && *Text && (WaveLengthPx > 0.0f));
		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		ImFont* Font = ImGui::GetFont();
		const float FontSize = ImGui::GetFontSize();

		const ImVec2 StartPos = ImGui::GetCursorScreenPos();
		ImVec2 Pen = StartPos;

		const float Time = ImGui::GetTime();
		const float InvWavelength = (1.0f / WaveLengthPx);

		for (const char* Ptr = Text; *Ptr; Ptr != nullptr)
		{
			if (*Ptr == '\n')
			{
				Pen.x = StartPos.x;
				Pen.y += FontSize;
				++Ptr;
				continue;
			}

			char Ch[2] = { *Ptr, 0 };
			Ptr++;

			const float AdvanceX = Font->CalcTextSizeA(FontSize, FLT_MAX, 0.0f, Ch).x;
			const float Phase = (Pen.x - Time * SpeedPxPerSec) * InvWavelength;
			const float Hue = Phase - std::floor(Phase);

			ImVec4 Col;
			ImGui::ColorConvertHSVtoRGB(Hue, Saturation, Value, Col.x, Col.y, Col.z);
			Col.w = 1.0f;

			DrawList->AddText(Font, FontSize, Pen, ImGui::ColorConvertFloat4ToU32(Col), Ch);
			Pen.x += AdvanceX;
		}

		/* Reserve layout space so following widgets align vertically. */
		const ImVec2 TextSize = ImGui::CalcTextSize(Text, nullptr, false, FLT_MAX);
		ImGui::Dummy(ImVec2(TextSize.x, TextSize.y));
	}

}
