#include "combo.h"

#include "core/window.h"
#include "core/input/keyboard.h"
#include "game/instance.h"
#include "renderer/color.h"
#include "renderer/font.h"
#include "renderer/renderer.h"
#include "ui_core.h"
#include "widgets.h"
#include "scene/scene.h"

namespace platformer2d::UI::Widget::Combo {

	void TextureModifier()
	{
		constexpr float ButtonPaddingY = 7.0f;
		constexpr ImVec2 ButtonSize(84, 42);
		constexpr float ItemWidth = 2.0f * ButtonSize.x;

		ImGui::Dummy(ImVec2(0, 12));
		static ETexture SelectedTexture = ETexture::White;
		if (TextureDropdown(SelectedTexture)) {
			LK_DEBUG("Selected: {}", Enum::ToString(SelectedTexture));
		}

		const ImVec2 Avail = ImGui::GetContentRegionAvail();
		if (const std::shared_ptr<CTexture> TextureRef = CRenderer::GetTexture(SelectedTexture); TextureRef != nullptr) {
			/* Texture preview. */
			ImGui::SameLine(0.0f, 12.0f);
			UI::ShiftCursorY(-4.0f);
			ImGui::Image(
				static_cast<ImU64>(TextureRef->GetID()),
				ImVec2(32.0f, 32.0f),
				ImVec2(0.0f, 1.0f), /* Uv0. */
				ImVec2(1.0f, 0.0f) /* Uv1. */
			);

			static constexpr std::string_view Marker = "assets/textures/";
			auto StripPrefix = [](const std::filesystem::path& Path) -> std::string
			{
				const std::string Str = Path.generic_string();
				const std::size_t Pos = Str.find(Marker);
				if (Pos != std::string::npos) {
					return Str.substr(Pos);
				}
				return Str;
			};

			ImGui::Dummy(ImVec2(0, 6));

			{
				UI::FScopedFont Font(UI::Font::Get(EFont::SourceSansPro, EFontSize::Regular, EFontModifier::BoldItalic));
				ImGui::Indent();
				ImGui::Text("Size:%-4s%dx%d", " ", TextureRef->GetWidth(), TextureRef->GetHeight());
				const std::string TrimmedPath = StripPrefix(TextureRef->GetFilePath());
				ImGui::Text("Path:%-4s%s", " ", TrimmedPath.c_str());
				ImGui::Unindent();
			}
		}

		ImGui::Dummy(ImVec2(0, 12));

		{
			UI::FScopedStyle FrameRounding(ImGuiStyleVar_FrameRounding, 10);
			UI::FScopedStyle FramePadding(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
			UI::FScopedStyle FrameBorder(ImGuiStyleVar_FrameBorderSize, 2.0f);
			UI::FScopedColor ButtonCol(ImGuiCol_Button, RGBA32::Titlebar::Default);
			UI::FScopedColor ButtonActiveCol(ImGuiCol_ButtonActive, RGBA32::LightGray);
			UI::FScopedColor ButtonHoveredCol(ImGuiCol_ButtonHovered, RGBA32::SelectionMuted);

			UI::ShiftCursorX(35.0f);
			ImGui::Text("Wrap");

			ImGui::SameLine((Avail.x * 0.50f) - (ItemWidth * 0.50f) + ButtonPaddingY);
			UI::ShiftCursorY(-ButtonPaddingY);
			if (ImGui::Button("Clamp", ButtonSize)) {
				CRenderer::GetTexture(SelectedTexture)->SetWrap(ETextureWrap::Clamp);
			}
			ImGui::SameLine(0.0f, ButtonPaddingY);
			UI::ShiftCursorY(-ButtonPaddingY);
			if (ImGui::Button("Repeat", ButtonSize)) {
				CRenderer::GetTexture(SelectedTexture)->SetWrap(ETextureWrap::Repeat);
			}

			ImGui::Dummy(ImVec2(0, 6));

			UI::ShiftCursorX(35.0f);
			ImGui::Text("Filter");

			ImGui::SameLine((Avail.x * 0.50f) - (ItemWidth * 0.50f) + ButtonPaddingY);
			UI::ShiftCursorY(-ButtonPaddingY);
			if (ImGui::Button("Linear", ButtonSize)) {
				CRenderer::GetTexture(SelectedTexture)->SetFilter(ETextureFilter::Linear);
			}

			ImGui::SameLine(0.0f, ButtonPaddingY);
			UI::ShiftCursorY(-ButtonPaddingY);
			if (ImGui::Button("Nearest", ButtonSize)) {
				CRenderer::GetTexture(SelectedTexture)->SetFilter(ETextureFilter::Nearest);
			}
		}
	}

	bool TextureDropdown(ETexture& Selected)
	{
		constexpr float ButtonPaddingY = 7.0f;
		constexpr ImVec2 ButtonSize(84, 42);
		constexpr float ItemWidth = 2.0f * ButtonSize.x;

		bool Updated = false;
		std::size_t SelectedIdx = std::to_underlying(Selected);
		static const auto Names = Enum::View<ETexture, const char*>();
		LK_ASSERT((SelectedIdx >= 0) && (SelectedIdx < Names.size()));

		static const std::string Label = "Texture";
		if (ImGui::GetCurrentTable() != nullptr) {
			ImGui::TableSetColumnIndex(0);
			UI::ShiftCursorX(17.0f);
			ImGui::Text(Label.c_str());

			ImGui::TableSetColumnIndex(1);
			UI::ShiftCursorX(7);
		} else {
			ImGui::Text(Label.c_str());
			ImGui::SameLine();
		}

		const float ComboItemWidth = ((ImGui::GetContentRegionAvail().x - 8.0f) / 2.0f);
		ImGui::SetNextItemWidth(ComboItemWidth);
		if (ImGui::BeginCombo("##Texture", Names[SelectedIdx])) {
			for (std::size_t Idx = 0; Idx < Names.size(); Idx++) {
				const bool IsSelected = (SelectedIdx == Idx);
				if (ImGui::Selectable(Names[Idx], IsSelected)) {
					SelectedIdx = Idx;
				}
			}

			if (SelectedIdx != std::to_underlying(Selected)) {
				Selected = static_cast<ETexture>(SelectedIdx);
				Updated = true;
			}

			ImGui::EndCombo();
		}

		return Updated;
	}

	bool BlendFunction(const float IndentX)
	{
		/* clang-format off */
#define UI_COMBO_OPTION(Value) { Value, #Value }
		/* clang-format on */
		static constexpr std::pair<GLenum, const char*> SourceBlendFuncs[] = {
			UI_COMBO_OPTION(GL_SRC_ALPHA),
			UI_COMBO_OPTION(GL_DST_ALPHA),
			UI_COMBO_OPTION(GL_ONE),
			UI_COMBO_OPTION(GL_ONE_MINUS_SRC_ALPHA),
			UI_COMBO_OPTION(GL_ONE_MINUS_DST_ALPHA),
			UI_COMBO_OPTION(GL_ONE_MINUS_CONSTANT_ALPHA),
		};
		static constexpr std::pair<GLenum, const char*> DestBlendFuncs[] = {
			UI_COMBO_OPTION(GL_SRC_ALPHA),
			UI_COMBO_OPTION(GL_DST_ALPHA),
			UI_COMBO_OPTION(GL_ONE_MINUS_SRC_ALPHA),
			UI_COMBO_OPTION(GL_ONE_MINUS_DST_ALPHA),
			UI_COMBO_OPTION(GL_ONE_MINUS_CONSTANT_ALPHA),
		};
#undef UI_COMBO_OPTION

		bool ShouldUpdate = false;

		static int SelectedSourceBlendFunc = -1;
		if (SelectedSourceBlendFunc == -1) {
			const int SourceFunc = CRenderer::GetBlendSource();
			switch (SourceFunc) {
				case GL_SRC_ALPHA:
					SelectedSourceBlendFunc = 0;
					break;
				case GL_DST_ALPHA:
					SelectedSourceBlendFunc = 1;
					break;
				case GL_ONE:
					SelectedSourceBlendFunc = 2;
					break;
				case GL_ONE_MINUS_SRC_ALPHA:
					SelectedSourceBlendFunc = 3;
					break;
				case GL_ONE_MINUS_DST_ALPHA:
					SelectedSourceBlendFunc = 4;
					break;
				case GL_ONE_MINUS_CONSTANT_ALPHA:
					SelectedSourceBlendFunc = 5;
					break;
			}
		}
		LK_ASSERT(SelectedSourceBlendFunc >= 0);

		static int SelectedDestBlendFunc = -1;
		if (SelectedDestBlendFunc == -1) {
			const int DestFunc = CRenderer::GetBlendDestination();
			switch (DestFunc) {
				case GL_SRC_ALPHA:
					SelectedDestBlendFunc = 0;
					break;
				case GL_DST_ALPHA:
					SelectedDestBlendFunc = 1;
					break;
				case GL_ONE_MINUS_SRC_ALPHA:
					SelectedDestBlendFunc = 2;
					break;
				case GL_ONE_MINUS_DST_ALPHA:
					SelectedDestBlendFunc = 3;
					break;
				case GL_ONE_MINUS_CONSTANT_ALPHA:
					SelectedDestBlendFunc = 4;
					break;
			}
		}
		LK_ASSERT(SelectedDestBlendFunc >= 0);

		ImGui::PushID("UI_BlendFunction");
		if (ImGui::BeginTable("##UI_BlendFunction", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip)) {
			const ImVec2 Avail = ImGui::GetContentRegionAvail();
			ImGuiStyle& Style = ImGui::GetStyle();
			ImGui::TableSetupColumn("L", 0, GAME_MENU_LABEL_COLUMN_WIDTH);
			ImGui::TableSetupColumn("V", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, Avail.x - GAME_MENU_LABEL_COLUMN_WIDTH);

			ImGui::TableNextRow();
			UI::Table::Label("Source", IndentX);

			ImGui::TableSetColumnIndex(1);
			UI::Font::Push(EFont::Roboto, EFontSize::Large, EFontModifier::Bold);
			ImGui::SetNextItemWidth(GAME_MENU_COLUMN_ITEM_WIDTH - 12.0f);
			if (ImGui::BeginCombo("##Source", SourceBlendFuncs[SelectedSourceBlendFunc].second)) {
				for (int N = 0; N < LK_ARRAYSIZE(SourceBlendFuncs); N++) {
					const bool bSelected = (SelectedSourceBlendFunc == N);
					if (ImGui::Selectable(SourceBlendFuncs[N].second, bSelected)) {
						SelectedSourceBlendFunc = N;
						LK_TRACE_TAG("UI", "Source: {}", SourceBlendFuncs[N].second);
						ShouldUpdate = true;
					}
				}
				ImGui::EndCombo();
			}
			UI::Font::Pop();

			ImGui::TableNextRow();
			UI::Table::Label("Destination", IndentX);

			ImGui::TableSetColumnIndex(1);
			UI::Font::Push(EFont::Roboto, EFontSize::Large, EFontModifier::Bold);
			ImGui::SetNextItemWidth(GAME_MENU_COLUMN_ITEM_WIDTH - 12.0f);
			if (ImGui::BeginCombo("##Destination", DestBlendFuncs[SelectedDestBlendFunc].second)) {
				for (int N = 0; N < LK_ARRAYSIZE(DestBlendFuncs); N++) {
					const bool bSelected = (SelectedDestBlendFunc == N);
					if (ImGui::Selectable(DestBlendFuncs[N].second, bSelected)) {
						SelectedDestBlendFunc = N;
						LK_TRACE_TAG("UI", "Destination: {}", DestBlendFuncs[N].second);
						ShouldUpdate = true;
					}
				}
				ImGui::EndCombo();
			}
			UI::Font::Pop();

			ImGui::EndTable();
		}
		ImGui::PopID(); /* ~UI_BlendFunction */

		if (ShouldUpdate) {
			LK_OpenGL_Verify(glBlendFunc(
				SourceBlendFuncs[SelectedSourceBlendFunc].first,
				DestBlendFuncs[SelectedDestBlendFunc].first));
		}

		return ShouldUpdate;
	}

	bool DepthFunction(const float IndentX)
	{
		/* clang-format off */
#define UI_COMBO_OPTION(Value) { Value, #Value }
		/* clang-format on */
		static constexpr std::pair<GLenum, const char*> Functions[] = {
			UI_COMBO_OPTION(GL_LESS),
			UI_COMBO_OPTION(GL_EQUAL),
			UI_COMBO_OPTION(GL_LEQUAL),
			UI_COMBO_OPTION(GL_GREATER),
			UI_COMBO_OPTION(GL_NOTEQUAL),
			UI_COMBO_OPTION(GL_GEQUAL),
			UI_COMBO_OPTION(GL_ALWAYS),
		};
#undef UI_COMBO_OPTION

		static constexpr float ItemWidth = 380.0f;
		bool ShouldUpdate = false;

		static int SelectedDepthFunc = -1;
		if (SelectedDepthFunc == -1) {
			const int Func = CRenderer::GetDepthFunction();
			switch (Func) {
				case GL_LESS:
					SelectedDepthFunc = 0;
					break;
				case GL_EQUAL:
					SelectedDepthFunc = 1;
					break;
				case GL_LEQUAL:
					SelectedDepthFunc = 2;
					break;
				case GL_GREATER:
					SelectedDepthFunc = 3;
					break;
				case GL_NOTEQUAL:
					SelectedDepthFunc = 4;
					break;
				case GL_GEQUAL:
					SelectedDepthFunc = 5;
					break;
				case GL_ALWAYS:
					SelectedDepthFunc = 6;
					break;
			}
		}

		if (SelectedDepthFunc == -1) {
			return false;
		}

		ImGui::PushID("UI_DepthFunction");
		if (ImGui::BeginTable("##UI_DepthFunction", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip)) {
			const ImVec2 Avail = ImGui::GetContentRegionAvail();
			ImGui::TableSetupColumn("Label", 0, GAME_MENU_LABEL_COLUMN_WIDTH);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, ImGui::GetContentRegionAvail().x - GAME_MENU_LABEL_COLUMN_WIDTH);

			ImGui::TableNextRow();
			UI::Table::Label("Depth", IndentX);

			ImGui::TableSetColumnIndex(1);
			ImGui::SetNextItemWidth(GAME_MENU_COLUMN_ITEM_WIDTH - 12.0f);
			UI::Font::Push(EFont::Roboto, EFontSize::Large, EFontModifier::Bold);
			if (ImGui::BeginCombo("##Depth", Functions[SelectedDepthFunc].second)) {
				for (int N = 0; N < LK_ARRAYSIZE(Functions); N++) {
					const bool bSelected = (SelectedDepthFunc == N);
					if (ImGui::Selectable(Functions[N].second, bSelected)) {
						SelectedDepthFunc = N;
						LK_TRACE_TAG("UI", "Depth: {}", Functions[N].second);
						ShouldUpdate = true;
					}
				}
				ImGui::EndCombo();
			}
			UI::Font::Pop();

			ImGui::EndTable();
		}
		ImGui::PopID();

		if (ShouldUpdate) {
			LK_OpenGL_Verify(glDepthFunc(Functions[SelectedDepthFunc].first));
		}

		return ShouldUpdate;
	}

}
