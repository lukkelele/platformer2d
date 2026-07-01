#include "textureinspector.h"

#include "core/log.h"
#include "core/profiler.h"
#include "core/window.h"
#include "renderer/renderer.h"
#include "renderer/ui/combo.h"
#include "renderer/ui/scoped.h"
#include "renderer/ui/ui.h"
#include "serialization/yaml.h"

namespace platformer2d::UI {

	static FTextureInspector TextureInspector;
	static bool bPendingFocusTexture = false;

	static constexpr float SPRITE_FIELD_WIDTH = 220.0f;

	bool IsTextureInspectorOpen()
	{
		return TextureInspector.bWindowOpen;
	}

	void OpenTextureInspector()
	{
		TextureInspector.bWindowOpen = true;
		bPendingFocusTexture = true;
		LK_TRACE_TAG("TextureInspector", "Open Texture Inspector");
	}

	void CloseTextureInspector()
	{
		TextureInspector.bWindowOpen = false;
		LK_TRACE_TAG("TextureInspector", "Close Texture Inspector");
	}

	void ToggleTextureInspector()
	{
		if (TextureInspector.bWindowOpen) {
			CloseTextureInspector();
		} else {
			OpenTextureInspector();
		}
	}

	static void RenderTexturePane()
	{
		auto& Pane = TextureInspector;
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 120.0f);
		ImGui::InputText("##TexturePath", Pane.PathBuf.data(), Pane.PathBuf.size());
		ImGui::SameLine();
		if (ImGui::Button("Browse", ImVec2(112.0f, 0.0f))) {
			std::filesystem::path Picked;
			if (PickImageFile(Picked)) {
				const std::string Str = Picked.generic_string();
				const std::size_t Copy = std::min<std::size_t>(Str.size(), Pane.PathBuf.size() - 1);
				std::memcpy(Pane.PathBuf.data(), Str.data(), Copy);
				Pane.PathBuf[Copy] = '\0';
				Pane.Spec.Path = Picked;
				Pane.Spec.Name = Picked.stem().generic_string();
				Pane.bDirty = true;
			}
		}

		ImGui::Spacing();

		constexpr float TextureFieldWidth = 220.0f;
		if (UI::BeginPropertyGrid(160.0f)) {
			UI::Table::NextRow();
			UI::Table::Label("Format");
			UI::Table::NextColumn();
			ImGui::SetNextItemWidth(TextureFieldWidth);
			UI::Combo("##Format", Enum::View<EImageFormat>(), Pane.Spec.Format);

			UI::Table::NextRow();
			UI::Table::Label("Wrap");
			UI::Table::NextColumn();
			ImGui::SetNextItemWidth(TextureFieldWidth);
			UI::Combo("##Wrap", Enum::View<ETextureWrap>(), Pane.Spec.SamplerWrap);

			UI::Table::NextRow();
			UI::Table::Label("Filter");
			UI::Table::NextColumn();
			ImGui::SetNextItemWidth(TextureFieldWidth);
			UI::Combo("##Filter", Enum::View<ETextureFilter>(), Pane.Spec.SamplerFilter);

			UI::Table::NextRow();
			UI::Checkbox("Flip Vertical", Pane.Spec.bFlipVertical);

			UI::Table::NextRow();
			UI::Checkbox("Invert", Pane.Spec.bInvert);

			UI::Table::NextRow();
			int Mips = static_cast<int>(Pane.Spec.Mips);
			UI::Table::Label("Mips");
			UI::Table::NextColumn();
			ImGui::SetNextItemWidth(TextureFieldWidth);
			if (ImGui::DragInt("##Mips", &Mips, 1.0f, 1, 16)) {
				Pane.Spec.Mips = static_cast<std::uint8_t>(std::clamp(Mips, 1, 16));
			}
			UI::EndPropertyGrid();
		}

		ImGui::Spacing();
		const bool HasPath = (Pane.PathBuf[0] != '\0');
		if (!HasPath) {
			ImGui::BeginDisabled();
		}
		if (ImGui::Button("Apply / Load", ImVec2(160.0f, 0.0f))) {
			Pane.Spec.Path = std::filesystem::path(Pane.PathBuf.data());
			if (Pane.Spec.Name.empty()) {
				Pane.Spec.Name = Pane.Spec.Path.stem().generic_string();
			}

			if (!std::filesystem::exists(Pane.Spec.Path)) {
				Pane.Status = "File does not exist: " + Pane.Spec.Path.generic_string();
				LK_ERROR_TAG("TextureInspector", "{}", Pane.Status);
			} else if (Pane.Texture && (Pane.Texture->GetFilePath() == Pane.Spec.Path)) {
				if (Pane.Texture->Reload(Pane.Spec)) {
					Pane.Status = "Reloaded";
					Pane.bDirty = false;
				} else {
					Pane.Status = "Reload failed";
				}
			} else {
				Pane.Texture = std::make_shared<CTexture>(Pane.Spec);
				Pane.Status = "Loaded";
				Pane.bDirty = false;
			}
		}
		if (!HasPath) {
			ImGui::EndDisabled();
		}
		ImGui::SameLine();
		if (Pane.Texture) {
			if (ImGui::Button("Clear", ImVec2(80.0f, 0.0f))) {
				Pane.Texture.reset();
				Pane.Status = "Cleared";
			}
		}

		if (!Pane.Status.empty()) {
			ImGui::SameLine();
			ImGui::TextDisabled("%s", Pane.Status.c_str());
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		if (Pane.Texture) {
			const std::uint32_t W = Pane.Texture->GetWidth();
			const std::uint32_t H = Pane.Texture->GetHeight();
			ImGui::Text("Size: %ux%u  Channels: %u  Mips: %u",
				W, H, Pane.Texture->GetChannels(), Pane.Texture->GetMips());
			ImGui::Text("Slot: %zu  ID: %u", Pane.Texture->GetSlot(), Pane.Texture->GetID());

			constexpr float MaxDisplayDim = 384.0f;
			const ImVec2 Avail = ImGui::GetContentRegionAvail();
			const float MaxW = std::max(64.0f, Avail.x);
			const float MaxH = std::max(64.0f, Avail.y - 16.0f);
			const float ScaleByAvail = std::min(MaxW / static_cast<float>(W), MaxH / static_cast<float>(H));
			const float ScaleByMax = MaxDisplayDim / static_cast<float>(std::max(W, H));
			const float AspectScale = std::min({ScaleByAvail, ScaleByMax, 1.0f});
			const ImVec2 ImgSize(static_cast<float>(W) * AspectScale, static_cast<float>(H) * AspectScale);
			ImGui::Image(static_cast<ImTextureID>(Pane.Texture->GetID()), ImgSize, ImVec2(0, 1), ImVec2(1, 0));
		} else {
			ImGui::TextDisabled("(No texture loaded)");
		}
	}

	void RenderTextureInspector()
	{
		LK_PROFILE_FUNC();
		if (!TextureInspector.bWindowOpen) {
			return;
		}

		RouteToCentralNode();
		if (bPendingFocusTexture) {
			ImGui::SetNextWindowFocus();
			bPendingFocusTexture = false;
		}
		if (UI::Begin(UI::PanelID::TextureInspector, &TextureInspector.bWindowOpen)) {
			RenderTexturePane();
			UI::End();
		}
	}

}

