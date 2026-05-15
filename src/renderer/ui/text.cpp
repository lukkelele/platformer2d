#include "ui.h"

#include "core/window.h"
#include "core/input/keyboard.h"
#include "game/instance.h"
#include "renderer/color.h"
#include "renderer/font.h"
#include "renderer/renderer.h"
#include "ui_core.h"
#include "widgets.h"
#include "scene/scene.h"

namespace platformer2d::UI::Text {

	void ColdGradient(const char* const Text, const float Speed)
	{
		const float Time = ImGui::GetTime() * Speed;

		const ImVec2 StartPos = ImGui::GetCursorScreenPos();
		ImFont* Font = ImGui::GetFont();
		const float FontSize = ImGui::GetFontSize();

		ImVec2 Pos = StartPos;
		ImDrawList* DrawList = ImGui::GetWindowDrawList();

		for (const char* Ptr = Text; *Ptr; Ptr++) {
			/* Create smooth oscillation between 0.0 and 1.0f */
			const float T = 0.50f * (std::sin(Time + (*Ptr) * 0.15f) + 1.0f);

			static const std::array<ImVec4, 4> Colors = {
				FColor::Convert<ImVec4>(FColor::White),
				FColor::Convert<ImVec4>(FColor::LightGray),
				FColor::Convert<ImVec4>(FColor::LightBlue),
				FColor::Convert<ImVec4>(FColor::Cyan),
			};

			/* Interpolate between colors. */
			const int Index1 = static_cast<int>(T * 3.0f);
			const int Index2 = std::min(Index1 + 1, 3);
			const float LocalT = (T * 3.0f) - static_cast<float>(Index1);

			ImVec4 Col;
			Col.x = Colors[Index1].x + (Colors[Index2].x - Colors[Index1].x) * LocalT;
			Col.y = Colors[Index1].y + (Colors[Index2].y - Colors[Index1].y) * LocalT;
			Col.z = Colors[Index1].z + (Colors[Index2].z - Colors[Index1].z) * LocalT;
			Col.w = 1.0f;

			const std::array<char, 2> Ch = {*Ptr, 0};
			DrawList->AddText(Font, FontSize, Pos, ImGui::ColorConvertFloat4ToU32(Col), Ch.data());
			Pos.x += Font->CalcTextSizeA(FontSize, FLT_MAX, 0.0f, Ch.data()).x;
		}

		ImGui::Dummy(ImVec2(Pos.x - StartPos.x, FontSize));
	}

	void RainbowGradient(const char* const Text, const float Speed)
	{
		const float Time = ImGui::GetTime() * 0.50f;

		const ImVec2 StartPos = ImGui::GetCursorScreenPos();
		ImFont* Font = ImGui::GetFont();
		const float FontSize = ImGui::GetFontSize();

		ImVec2 Pos = StartPos;
		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		for (const char* Ptr = Text; *Ptr; Ptr++) {
			float Hue = std::fmod(Time + (*Ptr) * Speed, 1.0f);
			ImVec4 Col;
			ImGui::ColorConvertHSVtoRGB(Hue, 1.0f, 1.0f, Col.x, Col.y, Col.z);
			Col.w = 1.0f;

			const std::array<char, 2> Ch = {*Ptr, 0};
			DrawList->AddText(Font, FontSize, Pos, ImGui::ColorConvertFloat4ToU32(Col), Ch.data());
			Pos.x += Font->CalcTextSizeA(FontSize, FLT_MAX, 0.0f, Ch.data()).x;
		}

		ImGui::Dummy(ImVec2(Pos.x - StartPos.x, FontSize));
	}

	void RainbowSynced(const char* const Text, const float WaveLengthPx, const float SpeedPxPerSec, const float Saturation, const float Value)
	{
		LK_ASSERT(Text && *Text && (WaveLengthPx > 0.0f));
		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		ImFont* Font = ImGui::GetFont();
		const float FontSize = ImGui::GetFontSize();

		const ImVec2 StartPos = ImGui::GetCursorScreenPos();
		ImVec2 Pen = StartPos;

		const float Time = ImGui::GetTime();
		const float InvWavelength = (1.0f / WaveLengthPx);

		for (const char* Ptr = Text; *Ptr;) {
			if (*Ptr == '\n') {
				Pen.x = StartPos.x;
				Pen.y += FontSize;
				++Ptr;
				continue;
			}

			std::array<char, 2> Ch = {*Ptr, 0};
			Ptr++;

			const float AdvanceX = Font->CalcTextSizeA(FontSize, FLT_MAX, 0.0f, Ch.data()).x;
			const float Phase = (Pen.x - Time * SpeedPxPerSec) * InvWavelength;
			const float Hue = Phase - std::floor(Phase);

			ImVec4 Col;
			ImGui::ColorConvertHSVtoRGB(Hue, Saturation, Value, Col.x, Col.y, Col.z);
			Col.w = 1.0f;

			DrawList->AddText(Font, FontSize, Pen, ImGui::ColorConvertFloat4ToU32(Col), Ch.data());
			Pen.x += AdvanceX;
		}

		/* Reserve layout space so following widgets align vertically. */
		const ImVec2 TextSize = ImGui::CalcTextSize(Text, nullptr, false, FLT_MAX);
		ImGui::Dummy(ImVec2(TextSize.x, TextSize.y));
	}

	void Shimmer(const char* const Text, const ImVec4& Base, const ImVec4& Highlight, const float SpeedPxPerSec, const float BandWidthPx)
	{
		LK_ASSERT(Text && *Text && (BandWidthPx > 0.0f));
		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		ImFont* Font = ImGui::GetFont();
		const float FontSize = ImGui::GetFontSize();
		const ImVec2 StartPos = ImGui::GetCursorScreenPos();

		const float TotalWidth = Font->CalcTextSizeA(FontSize, FLT_MAX, 0.0f, Text).x;
		const float SweepRange = TotalWidth + 2.0f * BandWidthPx;
		const float Time = ImGui::GetTime();
		const float BandCenter = -BandWidthPx + std::fmod(Time * SpeedPxPerSec, SweepRange);

		ImVec2 Pen = StartPos;
		for (const char* Ptr = Text; *Ptr; Ptr++) {
			const std::array<char, 2> Ch = {*Ptr, 0};
			const float AdvanceX = Font->CalcTextSizeA(FontSize, FLT_MAX, 0.0f, Ch.data()).x;
			const float CharCenter = (Pen.x - StartPos.x) + AdvanceX * 0.50f;
			const float Distance = std::fabs(CharCenter - BandCenter);
			const float T = (Distance < BandWidthPx) ? (1.0f - Distance / BandWidthPx) : 0.0f;
			const float Mix = T * T * (3.0f - 2.0f * T);

			ImVec4 Col;
			Col.x = Base.x + (Highlight.x - Base.x) * Mix;
			Col.y = Base.y + (Highlight.y - Base.y) * Mix;
			Col.z = Base.z + (Highlight.z - Base.z) * Mix;
			Col.w = Base.w + (Highlight.w - Base.w) * Mix;

			DrawList->AddText(Font, FontSize, Pen, ImGui::ColorConvertFloat4ToU32(Col), Ch.data());
			Pen.x += AdvanceX;
		}

		ImGui::Dummy(ImVec2(TotalWidth, FontSize));
	}

	void NeonPulse(const char* const Text, const ImVec4& Color, const float Speed, const float GlowRadiusPx, const std::size_t GlowSteps)
	{
		LK_ASSERT(Text && *Text && (GlowSteps > 0));
		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		ImFont* Font = ImGui::GetFont();
		const float FontSize = ImGui::GetFontSize();
		const ImVec2 StartPos = ImGui::GetCursorScreenPos();

		const float Time = ImGui::GetTime() * Speed;
		const float Pulse = 0.55f + 0.45f * (0.50f * (std::sin(Time) + 1.0f));

		const float StepRad = (2.0f * 3.14159265358979323846f) / static_cast<float>(GlowSteps);
		const float HaloAlpha = Color.w * 0.25f * Pulse;
		for (std::size_t Idx = 0; Idx < GlowSteps; Idx++) {
			const float Angle = static_cast<float>(Idx) * StepRad;
			const float Dx = std::cos(Angle) * GlowRadiusPx;
			const float Dy = std::sin(Angle) * GlowRadiusPx;
			const ImVec4 Halo(Color.x, Color.y, Color.z, HaloAlpha);
			DrawList->AddText(Font, FontSize, ImVec2(StartPos.x + Dx, StartPos.y + Dy),
				ImGui::ColorConvertFloat4ToU32(Halo), Text);
		}

		const ImVec4 CoreColor(Color.x, Color.y, Color.z, Color.w);
		DrawList->AddText(Font, FontSize, StartPos, ImGui::ColorConvertFloat4ToU32(CoreColor), Text);

		const ImVec2 TextSize = Font->CalcTextSizeA(FontSize, FLT_MAX, 0.0f, Text);
		ImGui::Dummy(ImVec2(TextSize.x, FontSize));
	}

	void Wave(const char* const Text, const ImVec4& Color, const float AmplitudePx,
		const float CharPhaseStep, const float Speed)
	{
		LK_ASSERT(Text && *Text);
		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		ImFont* Font = ImGui::GetFont();
		const float FontSize = ImGui::GetFontSize();
		const ImVec2 StartPos = ImGui::GetCursorScreenPos();

		const float Time = ImGui::GetTime() * Speed;
		const std::uint32_t ColorU32 = ImGui::ColorConvertFloat4ToU32(Color);

		ImVec2 Pen = StartPos;
		for (const char* Ptr = Text; *Ptr; Ptr++) {
			const std::array<char, 2> Ch = {*Ptr, 0};
			const float AdvanceX = Font->CalcTextSizeA(FontSize, FLT_MAX, 0.0f, Ch.data()).x;
			const float CharIndex = static_cast<float>(Ptr - Text);
			const float Phase = Time + CharIndex * CharPhaseStep;
			const float Dy = std::sin(Phase) * AmplitudePx;
			DrawList->AddText(Font, FontSize, ImVec2(Pen.x, Pen.y + Dy), ColorU32, Ch.data());
			Pen.x += AdvanceX;
		}

		ImGui::Dummy(ImVec2(Pen.x - StartPos.x, FontSize + AmplitudePx * 2.0f));
	}

	void ChromaticGlitch(const char* const Text, const ImVec4& Base, const float OffsetPx, const float Speed)
	{
		LK_ASSERT(Text && *Text);
		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		ImFont* Font = ImGui::GetFont();
		const float FontSize = ImGui::GetFontSize();
		const ImVec2 StartPos = ImGui::GetCursorScreenPos();

		const float Time = ImGui::GetTime() * Speed;
		const float Jitter = 0.50f * (std::sin(Time * 7.3f) + std::sin(Time * 3.1f));
		const float Dx = OffsetPx * (1.0f + 0.30f * Jitter);

		const std::uint32_t RedChannel = IM_COL32(255, 32, 64, 200);
		const std::uint32_t BlueChannel = IM_COL32(32, 200, 255, 200);
		const std::uint32_t BaseU32 = ImGui::ColorConvertFloat4ToU32(Base);

		DrawList->AddText(Font, FontSize, ImVec2(StartPos.x - Dx, StartPos.y), RedChannel, Text);
		DrawList->AddText(Font, FontSize, ImVec2(StartPos.x + Dx, StartPos.y), BlueChannel, Text);
		DrawList->AddText(Font, FontSize, StartPos, BaseU32, Text);

		const ImVec2 TextSize = Font->CalcTextSizeA(FontSize, FLT_MAX, 0.0f, Text);
		ImGui::Dummy(ImVec2(TextSize.x + 2.0f * OffsetPx, FontSize));
	}

}
