#include "spriteinspector.h"

#include "core/log.h"
#include "core/profiler.h"
#include "core/window.h"
#include "renderer/renderer.h"
#include "renderer/ui/combo.h"
#include "renderer/ui/scoped.h"
#include "renderer/ui/ui.h"
#include "serialization/yaml.h"

namespace platformer2d::UI {

	static FSpriteInspector SpriteInspector;
	static bool bPendingFocusSprite = false;

	static constexpr float SPRITE_FIELD_WIDTH = 220.0f;

	bool IsSpriteInspectorOpen()
	{
		return SpriteInspector.bWindowOpen;
	}

	void OpenSpriteInspector()
	{
		SpriteInspector.bWindowOpen = true;
		bPendingFocusSprite = true;
		LK_TRACE_TAG("SpriteInspector", "Open Sprite Inspector");
	}

	void CloseSpriteInspector()
	{
		SpriteInspector.bWindowOpen = false;
		LK_TRACE_TAG("SpriteInspector", "Close Sprite Inspector");
	}

	void ToggleSpriteInspector()
	{
		if (SpriteInspector.bWindowOpen) {
			CloseSpriteInspector();
		} else {
			OpenSpriteInspector();
		}
	}

	static std::string GeneratePreviewYaml()
	{
		const auto& Sprite = SpriteInspector;

		YAML::Emitter Out;
		Out << YAML::BeginMap;

		const std::string Name = Sprite.SheetName.data();
		Out << YAML::Key << "Name" << YAML::Value << (Name.empty() ? std::string("Unnamed") : Name);
		Out << YAML::Key << "Type" << YAML::Value << std::string(Enum::ToString(Sprite.SheetType));

		if (Sprite.Source == EAssetSpriteSource::Enum) {
			Out << YAML::Key << "Texture" << YAML::Value << std::string(Enum::ToString(Sprite.EnumTexture));
		} else {
			const std::string Stem("AdHoc");
			Out << YAML::Key << "Texture" << YAML::Value << Stem;
			Out << YAML::Comment("not in ETexture, AdHoc not implemented yet");
		}

		Out << YAML::Key << "TileSize" << YAML::Flow << YAML::BeginSeq
			<< static_cast<int>(Sprite.TileSize.x) << static_cast<int>(Sprite.TileSize.y)
			<< YAML::EndSeq;
		Out << YAML::Key << "Row" << YAML::Value << static_cast<int>(Sprite.DefaultRow);

		Out << YAML::Key << "Animations" << YAML::Value << YAML::BeginMap;
		for (const auto& Anim : Sprite.Animations) {
			if (Anim.Indices.empty()) {
				continue;
			}
			Out << YAML::Key << std::string(Enum::ToString(Anim.Frame));
			Out << YAML::Value << YAML::BeginMap;
			Out << YAML::Key << "Frames" << YAML::Flow << YAML::BeginSeq;
			for (const std::uint16_t Index : Anim.Indices) {
				Out << static_cast<int>(Index);
			}
			Out << YAML::EndSeq;
			Out << YAML::Key << "TicksPerFrame" << YAML::Value << static_cast<int>(Anim.TicksPerFrame);
			if (Anim.Row != Sprite.DefaultRow) {
				Out << YAML::Key << "Row" << YAML::Value << static_cast<int>(Anim.Row);
			}
			Out << YAML::EndMap;
		}
		Out << YAML::EndMap;
		Out << YAML::EndMap;

		return Out.c_str();
	}

	static int CalculateTileUnderMouse(const ImVec2& ImgMin, const ImVec2& ImgSize,
		const glm::vec2& TileSize, const std::uint32_t TextureW, const std::uint32_t TextureH,
		std::uint16_t& OutX, std::uint16_t& OutY)
	{
		const ImVec2 Mouse = ImGui::GetMousePos();
		if ((Mouse.x < ImgMin.x) || (Mouse.y < ImgMin.y)) {
			return -1;
		}
		if (Mouse.x >= (ImgMin.x + ImgSize.x) || Mouse.y >= (ImgMin.y + ImgSize.y)) {
			return -1;
		}

		const float LocalX = Mouse.x - ImgMin.x;
		const float LocalY = Mouse.y - ImgMin.y;
		const float CellW = (TileSize.x / static_cast<float>(TextureW)) * ImgSize.x;
		const float CellH = (TileSize.y / static_cast<float>(TextureH)) * ImgSize.y;
		if ((CellW <= 0.0f) || (CellH <= 0.0f)) {
			return -1;
		}

		OutX = static_cast<std::uint16_t>(LocalX / CellW);
		OutY = static_cast<std::uint16_t>(LocalY / CellH);
		return 0;
	}

	static void DrawSheetWithGrid(const std::shared_ptr<CTexture>& Texture)
	{
		auto& Inspector = SpriteInspector;
		const std::uint32_t TexW = Texture->GetWidth();
		const std::uint32_t TexH = Texture->GetHeight();
		if ((TexW == 0) || (TexH == 0) || (Inspector.TileSize.x <= 0.0f) || (Inspector.TileSize.y <= 0.0f)) {
			ImGui::TextDisabled("(Invalid texture or tile size)");
			return;
		}

		ImGui::SetNextItemWidth(220.0f);
		ImGui::SliderFloat("Zoom##Sheet", &Inspector.SheetZoom, 0.25f, 8.0f, "%.2fx");

		const ImVec2 ImgSize(static_cast<float>(TexW) * Inspector.SheetZoom, static_cast<float>(TexH) * Inspector.SheetZoom);
		ImGui::BeginChild("##SheetView", ImVec2(0.0f, std::max(128.0f, ImgSize.y + 16.0f)), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar);

		const ImVec2 ImgMin = ImGui::GetCursorScreenPos();
		UI::Image(Texture, ImgSize, ImVec2(0, 1), ImVec2(1, 0));
		const bool ImageHovered = ImGui::IsItemHovered();

		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		const float CellW = (Inspector.TileSize.x / static_cast<float>(TexW)) * ImgSize.x;
		const float CellH = (Inspector.TileSize.y / static_cast<float>(TexH)) * ImgSize.y;
		const std::uint32_t Cols = static_cast<std::uint32_t>(static_cast<float>(TexW) / Inspector.TileSize.x);
		const std::uint32_t Rows = static_cast<std::uint32_t>(static_cast<float>(TexH) / Inspector.TileSize.y);
		const std::uint32_t GridColor = FColor::White.WithAlpha(0.30f).As<std::uint32_t>();
		for (std::uint32_t Idx = 0; Idx <= Cols; Idx++) {
			const float X = ImgMin.x + Idx * CellW;
			DrawList->AddLine(ImVec2(X, ImgMin.y), ImVec2(X, ImgMin.y + ImgSize.y), GridColor);
		}
		for (std::uint32_t Idx = 0; Idx <= Rows; Idx++) {
			const float Y = ImgMin.y + Idx * CellH;
			DrawList->AddLine(ImVec2(ImgMin.x, Y), ImVec2(ImgMin.x + ImgSize.x, Y), GridColor);
		}

		const bool HasEditTarget = (Inspector.EditAnimIdx < Inspector.Animations.size());
		const std::uint16_t HighlightRow = HasEditTarget ? Inspector.Animations[Inspector.EditAnimIdx].Row : Inspector.DefaultRow;

		const float RowY = ImgMin.y + HighlightRow * CellH;
		DrawList->AddRect(
			ImVec2(ImgMin.x, RowY),
			ImVec2(ImgMin.x + ImgSize.x, RowY + CellH),
			RGBA32::NiceBlue,
			0.0f,
			ImDrawFlags_None,
			2.0f);

		/* Draw color for selected frame(s). */
		if (HasEditTarget) {
			const auto& Anim = Inspector.Animations[Inspector.EditAnimIdx];
			for (const std::uint16_t Idx : Anim.Indices) {
				const float X = ImgMin.x + Idx * CellW;
				DrawList->AddRectFilled(
					ImVec2(X, RowY),
					ImVec2(X + CellW, RowY + CellH),
					FColor::LightGreen.WithAlpha(0.25f).As<std::uint32_t>());
			}
		}

		/* Draw outline around currently hovered tile. */
		if (ImageHovered) {
			std::uint16_t TileX = 0;
			std::uint16_t TileY = 0;
			if (CalculateTileUnderMouse(ImgMin, ImgSize, Inspector.TileSize, TexW, TexH, TileX, TileY) == 0) {
				const float HX = ImgMin.x + TileX * CellW;
				const float HY = ImgMin.y + TileY * CellH;
				DrawList->AddRect(
					ImVec2(HX, HY),
					ImVec2(HX + CellW, HY + CellH),
					FColor::Yellow.WithAlpha(0.90f).As<std::uint32_t>(),
					0.0f,
					ImDrawFlags_None,
					2.0f);
				ImGui::SetTooltip("Tile (%u, %u)", TileX, TileY);

				if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && HasEditTarget) {
					FSpriteInspectorAnim& Anim = Inspector.Animations[Inspector.EditAnimIdx];
					if (Inspector.bAddDirectlyToAnim) {
						Anim.Row = TileY;
						Anim.Indices.push_back(TileX);
					}
				}
			}
		}

		ImGui::EndChild();
	}

	static void RenderAnimationList()
	{
		LK_PROFILER_SCOPED();
		auto& Inspector = SpriteInspector;

		if (ImGui::Button("+ Add Animation")) {
			FSpriteInspectorAnim NewAnim;
			NewAnim.Row = Inspector.DefaultRow;
			NewAnim.Frame = ESpriteFrame::Idle;
			for (std::size_t Idx = 0; Idx < std::to_underlying(ESpriteFrame::COUNT); Idx++) {
				const auto Candidate = static_cast<ESpriteFrame>(Idx);
				const bool Taken = std::any_of(Inspector.Animations.begin(), Inspector.Animations.end(), [Candidate](const FSpriteInspectorAnim& A)
				{
					return A.Frame == Candidate;
				});

				if (!Taken) {
					NewAnim.Frame = Candidate;
					break;
				}
			}
			Inspector.Animations.push_back(std::move(NewAnim));
			Inspector.EditAnimIdx = Inspector.Animations.size() - 1;
		}

		ImGui::SameLine();
		ImGui::Checkbox("Click tiles to append", &Inspector.bAddDirectlyToAnim);

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 2));
		std::size_t RemoveIdx = std::numeric_limits<std::size_t>::max();
		for (std::size_t Idx = 0; Idx < Inspector.Animations.size(); Idx++) {
			auto& Anim = Inspector.Animations[Idx];
			UI::FScopedID ScopedID(static_cast<int>(Idx));

			const bool IsEdit = (Idx == Inspector.EditAnimIdx);
			const bool IsPreview = (Idx == Inspector.PreviewAnimIdx);

			ImGui::PushStyleColor(ImGuiCol_Button, IsEdit ? FColor::LightBlue.WithAlpha(0.80f).As<std::uint32_t>() : FColor::Gray.WithAlpha(0.70f).As<std::uint32_t>());
			if (ImGui::Button("E")) {
				Inspector.EditAnimIdx = Idx;
			}
			ImGui::PopStyleColor();
			UI::SetTooltip("Edit (click tiles to append to this animation)");
			ImGui::SameLine();

			ImGui::PushStyleColor(ImGuiCol_Button, IsPreview ? FColor::LightGreen.WithAlpha(0.85f).As<std::uint32_t>() : FColor::Gray.WithAlpha(0.70f).As<std::uint32_t>());
			if (ImGui::Button("P")) {
				Inspector.PreviewAnimIdx = Idx;
				Inspector.PreviewTimeAccum = 0.0f;
			}
			ImGui::PopStyleColor();
			UI::SetTooltip("Set as preview animation");
			ImGui::SameLine();

			ImGui::SetNextItemWidth(140.0f);
			UI::Combo("##Frame", Enum::View<ESpriteFrame>(), Anim.Frame);
			ImGui::SameLine();

			int Row = static_cast<int>(Anim.Row);
			ImGui::SetNextItemWidth(60.0f);
			if (ImGui::DragInt("##Row", &Row, 1.0f, 0, 64, "R:%d")) {
				Anim.Row = static_cast<std::uint16_t>(std::clamp(Row, 0, 64));
			}
			ImGui::SameLine();

			int Tpf = static_cast<int>(Anim.TicksPerFrame);
			ImGui::SetNextItemWidth(70.0f);
			if (ImGui::DragInt("##TPF", &Tpf, 1.0f, 1, 240, "TPF:%d")) {
				Anim.TicksPerFrame = static_cast<std::uint16_t>(std::clamp(Tpf, 1, 240));
			}
			ImGui::SameLine();

			ImGui::PushStyleColor(ImGuiCol_Button, RGBA32::WineRed);
			if (ImGui::Button("X")) {
				RemoveIdx = Idx;
			}
			UI::SetTooltip("Remove animation");
			ImGui::PopStyleColor();

			if (ImGui::SmallButton("Clear")) {
				Anim.Indices.clear();
			}
			ImGui::SameLine();
			const bool HasFrames = !Anim.Indices.empty();
			if (!HasFrames) {
				ImGui::BeginDisabled();
			}
			if (ImGui::SmallButton("Pop")) {
				if (HasFrames) {
					Anim.Indices.pop_back();
				}
			}
			if (!HasFrames) {
				ImGui::EndDisabled();
			}
			ImGui::SameLine();

			std::string FramesText = "Frames: ";
			if (Anim.Indices.empty()) {
				FramesText += "(none)";
			} else {
				for (std::size_t I = 0; I < Anim.Indices.size(); I++) {
					if (I > 0) {
						FramesText += ", ";
					}
					FramesText += std::to_string(Anim.Indices[I]);
				}
			}
			ImGui::TextWrapped("%s", FramesText.c_str());

			ImGui::Separator();
		}
		ImGui::PopStyleVar();

		if (RemoveIdx != std::numeric_limits<std::size_t>::max()) {
			Inspector.Animations.erase(Inspector.Animations.begin() + RemoveIdx);
			if (Inspector.Animations.empty()) {
				Inspector.EditAnimIdx = 0;
				Inspector.PreviewAnimIdx = 0;
			} else {
				if (Inspector.EditAnimIdx >= Inspector.Animations.size()) {
					Inspector.EditAnimIdx = Inspector.Animations.size() - 1;
				}
				if (Inspector.PreviewAnimIdx >= Inspector.Animations.size()) {
					Inspector.PreviewAnimIdx = Inspector.Animations.size() - 1;
				}
			}
		}
	}

	static void RenderAnimationPreview(const std::shared_ptr<CTexture>& Texture)
	{
		LK_PROFILER_SCOPED();
		auto& Inspector = SpriteInspector;

		const bool HasAnim = (Inspector.PreviewAnimIdx < Inspector.Animations.size());
		if (!HasAnim) {
			ImGui::Indent();
			ImGui::TextDisabled("(No preview animation selected)");
			return;
		}
		const FSpriteInspectorAnim& Anim = Inspector.Animations[Inspector.PreviewAnimIdx];
		if (Anim.Indices.empty()) {
			ImGui::Indent();
			ImGui::TextDisabled("(Preview animation has no frames)");
			return;
		}

		const ImGuiIO& IO = ImGui::GetIO();
		Inspector.PreviewTimeAccum += IO.DeltaTime * Inspector.SimulatedTicksPerSec;
		const std::uint16_t TickIndex = static_cast<std::uint16_t>(Inspector.PreviewTimeAccum);

		const std::size_t FrameIdx = (TickIndex / Anim.TicksPerFrame) % Anim.Indices.size();
		const std::uint16_t TileX = Anim.Indices[FrameIdx];
		const std::uint16_t TileY = Anim.Row;

		const std::uint32_t TexW = Texture->GetWidth();
		const std::uint32_t TexH = Texture->GetHeight();
		const float TileW = Inspector.TileSize.x;
		const float TileH = Inspector.TileSize.y;

		const float U0 = (TileX * TileW) / static_cast<float>(TexW);
		const float U1 = ((TileX + 1) * TileW) / static_cast<float>(TexW);
		const float V0Tex = (TileY * TileH) / static_cast<float>(TexH);
		const float V1Tex = ((TileY + 1) * TileH) / static_cast<float>(TexH);
		const float V0Imgui = 1.0f - V0Tex;
		const float V1Imgui = 1.0f - V1Tex;

		const ImVec2 DisplaySize(TileW * Inspector.PreviewZoom, TileH * Inspector.PreviewZoom);
		UI::Image(Texture, DisplaySize, ImVec2(U0, V0Imgui), ImVec2(U1, V1Imgui));

		constexpr float FLOAT_WIDTH = 220.0f;
		ImGui::Text("Frame %zu/%zu  Tile (%u, %u)  TickIndex=%u TicksPerFrame=%u",
			FrameIdx + 1, Anim.Indices.size(), TileX, TileY, TickIndex, Anim.TicksPerFrame);
		ImGui::SetNextItemWidth(FLOAT_WIDTH);
		ImGui::SliderFloat("Simulated Ticks/Second##Preview", &Inspector.SimulatedTicksPerSec, 1.0f, 240.0f, "%.0f Hz");
		UI::SetTooltip("Real seconds -> simulated game ticks");

		ImGui::SetNextItemWidth(FLOAT_WIDTH);
		ImGui::SliderFloat("Zoom##Preview", &Inspector.PreviewZoom, 0.5f, 16.0f, "%.2fx");
	}

	static std::shared_ptr<CTexture> GetActiveSpriteTexture()
	{
		return CRenderer::GetTexture(SpriteInspector.EnumTexture);
	}

	static void RenderSpritePane()
	{
		LK_PROFILER_SCOPED();
		auto& Sprite = SpriteInspector;

		if (UI::BeginPropertyGrid(160.0f)) {
			UI::Table::NextRow();
			UI::Table::Label("Source");
			UI::Table::NextColumn();
			ImGui::SetNextItemWidth(SPRITE_FIELD_WIDTH);
			UI::Combo("##Source", Enum::View<EAssetSpriteSource>(), Sprite.Source);

			if (Sprite.Source == EAssetSpriteSource::Enum) {
				UI::Table::NextRow();
				UI::Table::Label("Texture");
				UI::Table::NextColumn();
				ImGui::SetNextItemWidth(SPRITE_FIELD_WIDTH);
				UI::Combo("##EnumTexture", Enum::View<ETexture>(), Sprite.EnumTexture);
			}

			UI::Table::NextRow();
			UI::Table::Label("Name");
			UI::Table::NextColumn();
			ImGui::SetNextItemWidth(SPRITE_FIELD_WIDTH);
			ImGui::InputText("##SheetName", Sprite.SheetName.data(), Sprite.SheetName.size());

			UI::Table::NextRow();
			UI::Table::Label("Type");
			UI::Table::NextColumn();
			ImGui::SetNextItemWidth(SPRITE_FIELD_WIDTH);
			UI::Combo("##SheetType", Enum::View<ESpriteSheetType>(), Sprite.SheetType);

			UI::Table::NextRow();
			UI::Table::Label("Tile Size");
			UI::Table::NextColumn();
			ImGui::SetNextItemWidth(SPRITE_FIELD_WIDTH);
			ImGui::DragFloat2("##TileSize", &Sprite.TileSize.x, 1.0f, 1.0f, 4096.0f, "%.0f");

			UI::Table::NextRow();
			int Row = static_cast<int>(Sprite.DefaultRow);
			UI::Table::Label("Default Row");
			UI::Table::NextColumn();
			ImGui::SetNextItemWidth(SPRITE_FIELD_WIDTH);
			if (ImGui::DragInt("##DefaultRow", &Row, 1.0f, 0, 64)) {
				Sprite.DefaultRow = static_cast<std::uint16_t>(std::clamp(Row, 0, 64));
			}
			UI::EndPropertyGrid();
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		std::shared_ptr<CTexture> Texture = GetActiveSpriteTexture();
		if (!Texture) {
			ImGui::TextDisabled("(Source texture is not available)");
			return;
		}

		const ImVec2 Avail = ImGui::GetContentRegionAvail();
		constexpr float SpriteReservedH = 28.0f;
		constexpr float SplitterThickness = 5.0f;
		constexpr float MinLeft = 260.0f;
		constexpr float MinRight = 220.0f;
		constexpr float MinSheet = 140.0f;
		constexpr float MinAnims = 140.0f;
		const float SplitHeight = std::max(280.0f, Avail.y - SpriteReservedH);

		float& LeftW = Sprite.LeftColWidth;
		LeftW = std::clamp(LeftW, MinLeft, std::max(MinLeft, Avail.x - MinRight - SplitterThickness));

		float& SheetH = Sprite.SheetHeight;
		SheetH = std::clamp(SheetH, MinSheet, std::max(MinSheet, SplitHeight - MinAnims - SplitterThickness));
		const float AnimsH = std::max(MinAnims, SplitHeight - SheetH - SplitterThickness);

		ImGui::BeginChild("##SpriteLeftCol", ImVec2(LeftW, SplitHeight), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
		ImGui::BeginChild("##SpriteSheet", ImVec2(0.0f, SheetH));
		DrawSheetWithGrid(Texture);
		ImGui::EndChild();

		HSplitter("##SheetHSplit", &SheetH, SplitterThickness, ImGui::GetContentRegionAvail().x);
		ImGui::BeginChild("##SpriteAnimsChild", ImVec2(0.0f, AnimsH));
		ImGui::Text("Animations");
		RenderAnimationList();
		ImGui::EndChild(); /* SheetAnimsSplit */
		ImGui::EndChild(); /* SpriteLeftCol */

		ImGui::SameLine(0.0f, 0.0f);
		VSplitter("##SpriteVSplit", &LeftW, SplitterThickness, ImGui::GetContentRegionAvail().x);
		ImGui::SameLine(0.0f, 0.0f);

		ImGui::BeginChild("##SpritePreviewChild", ImVec2(0.0f, SplitHeight));
		ImGui::SeparatorText("Preview");
		RenderAnimationPreview(Texture);
		ImGui::EndChild();

		ImGui::Spacing();
		ImGui::SetNextItemOpen(false, ImGuiCond_FirstUseEver);
		if (ImGui::CollapsingHeader(".lsprite (preview only)")) {
			const std::string Yaml = GeneratePreviewYaml();
			if (ImGui::Button("Dump to log")) {
				LK_INFO_TAG("SpriteInspector", "Generated .lsprite preview:\n{}", Yaml);
			}
			ImGui::Spacing();
			static std::array<char, 8192> YamlBuf{};
			const std::size_t Copy = std::min<std::size_t>(Yaml.size(), YamlBuf.size() - 1);
			std::memcpy(YamlBuf.data(), Yaml.data(), Copy);
			YamlBuf[Copy] = '\0';
			ImGui::InputTextMultiline("##YamlPreview", YamlBuf.data(), YamlBuf.size(),
				ImVec2(-FLT_MIN, std::max(160.0f, ImGui::GetContentRegionAvail().y - 12.0f)),
				ImGuiInputTextFlags_ReadOnly);
		}
	}

	void RenderSpriteInspector()
	{
		LK_PROFILER_SCOPED();
		if (!SpriteInspector.bWindowOpen) {
			return;
		}

		RouteToCentralNode();
		if (bPendingFocusSprite) {
			LK_DEBUG_TAG("SpriteInspector", "Pending focus");
			ImGui::SetNextWindowFocus();
			bPendingFocusSprite = false;
		}

		if (UI::Begin(UI::PanelID::SpriteInspector, &SpriteInspector.bWindowOpen)) {
			RenderSpritePane();
			UI::End();
		}
	}

}
