#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

namespace platformer2d::UI::Text {

	void ColdGradient(const char* Text, float Speed = 2.0f);
	void RainbowGradient(const char* Text, float Speed = 0.15f);
	void RainbowSynced(const char* Text, float WaveLengthPx = 180.0f, float SpeedPxPerSec = 30.0f, float Saturation = 1.0f, float Value = 1.0f);
	void Shimmer(const char* Text, const ImVec4& Base, const ImVec4& Highlight, float SpeedPxPerSec = 240.0f, float BandWidthPx = 120.0f);
	void NeonPulse(const char* Text, const ImVec4& Color, float Speed = 2.5f, float GlowRadiusPx = 2.0f, std::size_t GlowSteps = 8);
	void Wave(const char* Text, const ImVec4& Color, float AmplitudePx = 3.0f, float CharPhaseStep = 0.55f, float Speed = 4.0f);
	void ChromaticGlitch(const char* Text, const ImVec4& Base, float OffsetPx = 1.5f, float Speed = 6.0f);

}

