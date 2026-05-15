#pragma once

#include <cstdint>

#include <glm/glm.hpp>
#include <imgui/imgui.h>

#include "renderer/color.h"
#include "renderer/texture.h"

namespace platformer2d::UI {

	enum class EMenuAnchor : std::uint8_t
	{
		Center,
		LeftTop,
		LeftCenter,
		LeftBottom,
		RightTop,
		RightCenter,
		RightBottom,
		COUNT
	};

	struct FMainMenuBackground
	{
		struct FGradient
		{
			std::uint32_t TopLeft = FColor::Convert<std::uint32_t>(FColor::Black);
			std::uint32_t TopRight = FColor::Convert<std::uint32_t>(glm::vec4(0.450f, 0.200f, 0.550f, 1.0f));
			std::uint32_t BottomLeft = FColor::Convert<std::uint32_t>(glm::vec4(0.300f, 0.100f, 0.700f, 1.0f));
			std::uint32_t BottomRight = FColor::Convert<std::uint32_t>(glm::vec4(0.60f, 0.60f, 0.60f, 1.0f));
		} Gradient;

		struct FGrid
		{
			bool Enabled = true;
			std::uint32_t Color = FColor::Convert<std::uint32_t>(glm::vec4(1.0f, 1.0f, 1.0f, 0.045f));
			float Spacing = 64.0f;
		} Grid;

		struct FGlow
		{
			bool Enabled = false;
			std::uint32_t Color = FColor::Convert<std::uint32_t>(glm::vec4(1.000f, 0.400f, 0.650f, 0.120f));
			float RadiusFactor = 0.55f;
			int Segments = 64;
		} Glow;

		struct FAccent
		{
			bool Enabled = true;
			ETexture Texture = ETexture::Player;
			EMenuAnchor Anchor = EMenuAnchor::RightCenter;
			float ScaleFactor = 0.45f;
			float MarginX = 120.0f;
			float MarginY = 0.0f;
			float Opacity = 1.0f;
			std::uint32_t Tint = RGBA32::White;
			bool PreserveAspect = true;

			glm::vec2 TilePos = glm::vec2(0.0f, 2.0f);
			glm::vec2 TileSize = glm::vec2(32.0f, 32.0f);

			ImVec2 UV0 = ImVec2(0.0f, 0.0f);
			ImVec2 UV1 = ImVec2(1.0f, 1.0f);
		} Accent;
	};
	inline FMainMenuBackground MainMenuBackground{};

	void DrawMainMenuBackground(ImGuiViewport* Viewport, const FMainMenuBackground& Style = MainMenuBackground);
}
