#include "mainmenustyle.h"

#include <algorithm>

#include "renderer/renderer.h"
#include "renderer/texture.h"

namespace platformer2d::UI {

	static ImVec2 ResolveAnchorPos(const EMenuAnchor Anchor, const ImVec2& ViewportMin,
		const ImVec2& ViewportSize, const ImVec2& ImageSize, const float MarginX, const float MarginY)
	{
		float X = 0.0f;
		float Y = 0.0f;
		switch (Anchor) {
			case EMenuAnchor::Center:
				X = (ViewportSize.x - ImageSize.x) * 0.50f;
				Y = (ViewportSize.y - ImageSize.y) * 0.50f;
				break;
			case EMenuAnchor::LeftTop:
				X = MarginX;
				Y = MarginY;
				break;
			case EMenuAnchor::LeftCenter:
				X = MarginX;
				Y = (ViewportSize.y - ImageSize.y) * 0.50f + MarginY;
				break;
			case EMenuAnchor::LeftBottom:
				X = MarginX;
				Y = ViewportSize.y - ImageSize.y - MarginY;
				break;
			case EMenuAnchor::RightTop:
				X = ViewportSize.x - ImageSize.x - MarginX;
				Y = MarginY;
				break;
			case EMenuAnchor::RightCenter:
				X = ViewportSize.x - ImageSize.x - MarginX;
				Y = (ViewportSize.y - ImageSize.y) * 0.50f + MarginY;
				break;
			case EMenuAnchor::RightBottom:
				X = ViewportSize.x - ImageSize.x - MarginX;
				Y = ViewportSize.y - ImageSize.y - MarginY;
				break;
			default:
				break;
		}

		return ImVec2(ViewportMin.x + X, ViewportMin.y + Y);
	}

	static void DrawAccent(ImDrawList* const DrawList, const ImVec2& ViewportMin,
		const ImVec2& ViewportSize, const FMainMenuBackground::FAccent& Accent)
	{
		if (!Accent.Enabled) {
			return;
		}

		std::shared_ptr<CTexture> Texture = CRenderer::GetTexture(Accent.Texture);
		if (!Texture) {
			return;
		}

		const float TexW = static_cast<float>(Texture->GetWidth());
		const float TexH = static_cast<float>(Texture->GetHeight());
		if ((TexW <= 0.0f) || (TexH <= 0.0f)) {
			return;
		}

		const bool UseFrame = (Accent.TileSize.x > 0.0f) && (Accent.TileSize.y > 0.0f);
		const float SourceW = UseFrame ? Accent.TileSize.x : TexW;
		const float SourceH = UseFrame ? Accent.TileSize.y : TexH;

		float ImageH = ViewportSize.y * Accent.ScaleFactor;
		float ImageW = SourceW * (ImageH / SourceH);
		if (!Accent.PreserveAspect) {
			ImageW = ViewportSize.x * Accent.ScaleFactor;
		}

		ImVec2 UV0 = Accent.UV0;
		ImVec2 UV1 = Accent.UV1;
		if (UseFrame) {
			const float InvTexW = 1.0f / TexW;
			const float InvTexH = 1.0f / TexH;
			UV0.x = (Accent.TilePos.x * Accent.TileSize.x) * InvTexW;
			UV1.x = ((Accent.TilePos.x + 1.0f) * Accent.TileSize.x) * InvTexW;
			UV0.y = ((Accent.TilePos.y + 1.0f) * Accent.TileSize.y) * InvTexH;
			UV1.y = (Accent.TilePos.y * Accent.TileSize.y) * InvTexH;
		}

		const ImVec2 ImageSize(ImageW, ImageH);
		const ImVec2 TopLeft = ResolveAnchorPos(Accent.Anchor, ViewportMin, ViewportSize,
			ImageSize, Accent.MarginX, Accent.MarginY);
		const ImVec2 BottomRight(TopLeft.x + ImageW, TopLeft.y + ImageH);

		const float OpacityClamped = (Accent.Opacity < 0.0f) ? 0.0f : ((Accent.Opacity > 1.0f) ? 1.0f : Accent.Opacity);
		const std::uint8_t Alpha = static_cast<std::uint8_t>(OpacityClamped * 255.0f);
		const std::uint32_t Tint = (Accent.Tint & 0x00FFFFFF) | (static_cast<std::uint32_t>(Alpha) << 24);

		DrawList->AddImage(
			static_cast<std::uint64_t>(Texture->GetID()),
			TopLeft, BottomRight,
			UV0, UV1,
			Tint);
	}

	void DrawMainMenuBackground(ImGuiViewport* const Viewport, const FMainMenuBackground& Style)
	{
		ImDrawList* DrawList = ImGui::GetBackgroundDrawList(Viewport);
		const ImVec2 Min = Viewport->Pos;
		const ImVec2 Size = Viewport->Size;
		const ImVec2 Max(Min.x + Size.x, Min.y + Size.y);

		DrawList->AddRectFilledMultiColor(
			Min, Max,
			Style.Gradient.TopLeft,
			Style.Gradient.TopRight,
			Style.Gradient.BottomRight,
			Style.Gradient.BottomLeft);

		if (Style.Grid.Enabled && (Style.Grid.Spacing > 0.0f)) {
			for (float X = Min.x; X < Max.x; X += Style.Grid.Spacing) {
				DrawList->AddLine(ImVec2(X, Min.y), ImVec2(X, Max.y), Style.Grid.Color);
			}
			for (float Y = Min.y; Y < Max.y; Y += Style.Grid.Spacing) {
				DrawList->AddLine(ImVec2(Min.x, Y), ImVec2(Max.x, Y), Style.Grid.Color);
			}
		}

		if (Style.Glow.Enabled) {
			const ImVec2 Center((Min.x + Max.x) * 0.50f, (Min.y + Max.y) * 0.50f);
			const float GlowRadius = std::min(Size.x, Size.y) * Style.Glow.RadiusFactor;
			DrawList->AddCircleFilled(Center, GlowRadius, Style.Glow.Color, Style.Glow.Segments);
		}

		DrawAccent(DrawList, Min, Size, Style.Accent);
	}

}
