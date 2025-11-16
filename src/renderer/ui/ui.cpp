#include "ui.h"

#include "core/window.h"
#include "core/input/keyboard.h"
#include "game/gameinstance.h"
#include "renderer/color.h"
#include "renderer/font.h"
#include "renderer/renderer.h"
#include "ui_core.h"

namespace platformer2d::UI {

	FOnGameMenuOpened OnGameMenuOpened;

	namespace 
	{
		/* @todo: Use global config */
		constexpr float LABEL_COLUMN_WIDTH = 190.0f;
		constexpr float LABEL_INDENT_WIDTH = 24.0f;
		constexpr float COLUMN_ITEM_WIDTH = 410.0f;
	}

	bool BlendFunction()
	{
		#define UI_COMBO_OPTION(Value) { Value, #Value }
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

		bool bSetBlendFunc = false;

		static int SelectedSourceBlendFunc = -1;
		if (SelectedSourceBlendFunc == -1)
		{
			const int SourceFunc = CRenderer::GetBlendSource();
			switch (SourceFunc)
			{
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
		if (SelectedDestBlendFunc == -1)
		{
			const int DestFunc = CRenderer::GetBlendDestination();
			switch (DestFunc)
			{
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
		ImGui::BeginTable("##VectorControl", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip);
		ImGui::TableSetupColumn("LabelColumn", 0, LABEL_COLUMN_WIDTH);
		ImGui::TableSetupColumn("ValueColumn", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, ImGui::GetContentRegionAvail().x - LABEL_COLUMN_WIDTH);

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		UI::ShiftCursor(LABEL_INDENT_WIDTH, 4.0f);
		ImGui::Text("Source");

		ImGui::TableSetColumnIndex(1);
		UI::ShiftCursor(0.0f, 4.0f);
		ImGui::SetNextItemWidth(COLUMN_ITEM_WIDTH);
		if (ImGui::BeginCombo("##Source", SourceBlendFuncs[SelectedSourceBlendFunc].second))
		{
			for (int N = 0; N < LK_ARRAYSIZE(SourceBlendFuncs); N++)
			{
				const bool bSelected = (SelectedSourceBlendFunc == N);
				if (ImGui::Selectable(SourceBlendFuncs[N].second, bSelected))
				{
					SelectedSourceBlendFunc = N;
					LK_TRACE_TAG("UI", "Source: {}", SourceBlendFuncs[N].second);
					bSetBlendFunc = true;
				}
			}
			ImGui::EndCombo();
		}

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		UI::ShiftCursor(LABEL_INDENT_WIDTH, 4.0f);
		ImGui::Text("Destination");

		ImGui::TableSetColumnIndex(1);
		UI::ShiftCursor(0.0f, 4.0f);
		ImGui::SetNextItemWidth(COLUMN_ITEM_WIDTH);
		if (ImGui::BeginCombo("##Destination", DestBlendFuncs[SelectedDestBlendFunc].second))
		{
			for (int N = 0; N < LK_ARRAYSIZE(DestBlendFuncs); N++)
			{
				const bool bSelected = (SelectedDestBlendFunc == N);
				if (ImGui::Selectable(DestBlendFuncs[N].second, bSelected))
				{
					SelectedDestBlendFunc = N;
					LK_TRACE_TAG("UI", "Destination: {}", DestBlendFuncs[N].second);
					bSetBlendFunc = true;
				}
			}
			ImGui::EndCombo();
		}

		ImGui::EndTable();
		ImGui::PopID(); /* ~UI_BlendFunction */

		if (bSetBlendFunc)
		{
			LK_OpenGL_Verify(glBlendFunc(
				SourceBlendFuncs[SelectedSourceBlendFunc].first,
				DestBlendFuncs[SelectedDestBlendFunc].first
			));
		}

		return bSetBlendFunc;
	}

	bool DepthFunction()
	{
		#define UI_COMBO_OPTION(Value) { Value, #Value }
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
		if (SelectedDepthFunc == -1)
		{
			const int Func = CRenderer::GetDepthFunction();
			switch (Func)
			{
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

		if (SelectedDepthFunc == -1)
		{
			return false;
		}

		ImGui::PushID("UI_DepthFunction");
		ImGui::BeginTable("##VectorControl", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip);
		ImGui::TableSetupColumn("LabelColumn", 0, LABEL_COLUMN_WIDTH);
		ImGui::TableSetupColumn("ValueColumn", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, ImGui::GetContentRegionAvail().x - LABEL_COLUMN_WIDTH);

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		UI::ShiftCursor(LABEL_INDENT_WIDTH, 4.0f);
		ImGui::Text("Depth");

		ImGui::TableSetColumnIndex(1);
		UI::ShiftCursor(0.0f, 4.0f);
		ImGui::SetNextItemWidth(COLUMN_ITEM_WIDTH);
		if (ImGui::BeginCombo("##Depth", Functions[SelectedDepthFunc].second))
		{
			for (int N = 0; N < LK_ARRAYSIZE(Functions); N++)
			{
				const bool bSelected = (SelectedDepthFunc == N);
				if (ImGui::Selectable(Functions[N].second, bSelected))
				{
					SelectedDepthFunc = N;
					LK_TRACE_TAG("UI", "Depth: {}", Functions[N].second);
					ShouldUpdate = true;
				}
			}
			ImGui::EndCombo();
		}

		ImGui::EndTable();
		ImGui::PopID();

		if (ShouldUpdate)
		{
			LK_OpenGL_Verify(glDepthFunc(Functions[SelectedDepthFunc].first));
		}

		return ShouldUpdate;
	}

	void DrawGizmo(const int Operation, CActor& Actor, const glm::mat4& ViewMatrix, const glm::mat4& ProjectionMatrix, const glm::vec3& CameraPos)
	{
		static_assert(std::is_same_v<std::decay_t<decltype(Operation)>, std::underlying_type_t<ImGuizmo::OPERATION>>);
		ImGuizmo::SetOrthographic(true);
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

		FTransformComponent& TC = Actor.GetTransformComponent();
		glm::mat4 TransformMatrix = TC.GetTransform();

		ImGuizmo::Manipulate(
			glm::value_ptr(ViewMatrix),
			glm::value_ptr(ProjectionMatrix),
			static_cast<ImGuizmo::OPERATION>(Operation),
			ImGuizmo::WORLD, //ImGuizmo::LOCAL,
			glm::value_ptr(TransformMatrix),
			nullptr,
			nullptr
		);

		if (ImGuizmo::IsUsing())
		{
			glm::vec3 Translation;
			glm::vec3 Scale;
			glm::quat Rotation;
			Math::DecomposeTransform(TransformMatrix, Translation, Rotation, Scale);
			LK_UNUSED(Scale, Rotation);

			Actor.SetPosition(Translation);
		}
	}

	void ColdTextGradient(const char* Text, const float Speed)
	{
		const float Time = ImGui::GetTime() * Speed;

		const ImVec2 StartPos = ImGui::GetCursorScreenPos();
		ImFont* Font = ImGui::GetFont();
		const float FontSize = ImGui::GetFontSize();

		ImVec2 Pos = StartPos;
		ImDrawList* DrawList = ImGui::GetWindowDrawList();

		for (const char* Ptr = Text; *Ptr; Ptr++)
		{
			/* Create smooth oscillation between 0.0 and 1.0f */
			const float T = 0.50f * (std::sin(Time + (*Ptr) * 0.15f) + 1.0f);

			static const ImVec4 Colors[] =
			{
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

			const char Character[2] = {*Ptr, 0};
			DrawList->AddText(Font, FontSize, Pos, ImGui::ColorConvertFloat4ToU32(Col), Character);
			Pos.x += Font->CalcTextSizeA(FontSize, FLT_MAX, 0.0f, Character).x;
		}

		ImGui::Dummy(ImVec2(Pos.x - StartPos.x, FontSize));
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

		for (const char* Ptr = Text; *Ptr;)
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
