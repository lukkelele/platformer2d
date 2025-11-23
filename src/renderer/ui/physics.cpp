#include "ui.h"

#include "core/window.h"
#include "core/input/keyboard.h"
#include "game/gameinstance.h"
#include "renderer/color.h"
#include "renderer/font.h"
#include "renderer/renderer.h"
#include "ui_core.h"
#include "widgets.h"
#include "scene/scene.h"

namespace platformer2d::UI {

	void Aggregate(const FPhysicsBodyData& Data, FBodySpecification& BodySpec)
	{
		BodySpec.Position = { Data.Position.x, Data.Position.y };
		BodySpec.Friction = Data.Friction;
		BodySpec.Density = Data.Density;
		BodySpec.GravityScale = Data.GravityScale;
		BodySpec.LinearVelocity = Data.LinearVelocity;
		BodySpec.LinearDamping = Data.LinearDamping;
		BodySpec.AngularVelocity = Data.AngularVelocity;
		BodySpec.AngularDamping = Data.AngularDamping;
		BodySpec.DirForce = Data.DirForce;
		BodySpec.JumpImpulse = Data.JumpImpulse;
		BodySpec.bSensor = Data.BodyFlag.bSensorEvents;

		/* Body flags. */
		int BodyFlags = EBodyFlag::EBodyFlag_None;
		if (Data.BodyFlag.bPreSolveEvents) BodyFlags |= EBodyFlag::EBodyFlag_PreSolveEvents;
		if (Data.BodyFlag.bContactEvents) BodyFlags |= EBodyFlag::EBodyFlag_ContactEvents;
		if (Data.BodyFlag.bSensorEvents) BodyFlags |= EBodyFlag::EBodyFlag_SensorEvents;
		if (Data.BodyFlag.bBullet) BodyFlags |= EBodyFlag::EBodyFlag_Bullet;
		BodySpec.Flags = static_cast<EBodyFlag>(BodyFlags);

		/* Motion lock flags. */
		int MotionLockFlags = EMotionLock::EMotionLock_None;
		if (Data.MotionLock.All)
		{
			MotionLockFlags = EMotionLock::EMotionLock_All;
		}
		else
		{
			if (Data.MotionLock.X) MotionLockFlags |= std::to_underlying(EMotionLock::EMotionLock_X);
			if (Data.MotionLock.Y) MotionLockFlags |= std::to_underlying(EMotionLock::EMotionLock_Y);
			if (Data.MotionLock.Z) MotionLockFlags |= std::to_underlying(EMotionLock::EMotionLock_Z);
		}
		BodySpec.MotionLock = static_cast<EMotionLock>(MotionLockFlags);
	}

	void PhysicsBodyMenu(FPhysicsBodyData& Data)
	{
		static constexpr float ButtonPaddingY = 7.0f;
		static constexpr ImVec2 ButtonSize(84, 42);
		static constexpr float ItemWidth = 2.0f * ButtonSize.x;
		static constexpr float ColWidth = 180.0f;

		static constexpr std::array<EBodyType, std::to_underlying(EBodyType::COUNT)> BodyTypes = {
			EBodyType::Static,
			EBodyType::Dynamic,
			EBodyType::Kinematic,
		};

		ImGui::PushID("PhysicsBodyMenu");
		static std::size_t SelectedIdx = 0;
		LK_ASSERT((SelectedIdx >= 0) && (SelectedIdx < BodyTypes.size()));

		const ImVec2 Avail = ImGui::GetContentRegionAvail();

		/************************
		 * Body Type.
		 ************************/
		{
			ImGui::BeginTable("##BodyTypeTable", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip);
			ImGui::TableSetupColumn("L", 0, ColWidth);
			ImGui::TableSetupColumn("V", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, ImGui::GetContentRegionAvail().x - ColWidth);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			UI::ShiftCursor(17.0f, 7.0f);
			ImGui::Text("Body Type");

			ImGui::TableSetColumnIndex(1);
			UI::ShiftCursor(7.0f, 0.0f);
			const float ComboItemWidth = ((ImGui::GetContentRegionAvail().x - 8.0f) / 2.0f);
			ImGui::SetNextItemWidth(ComboItemWidth);
			const char* Selected = Enum::ToString(BodyTypes[SelectedIdx]);
			if (ImGui::BeginCombo("##BodyType", Selected))
			{
				for (int Idx = 0; Idx < BodyTypes.size(); Idx++)
				{
					const char* Option = Enum::ToString(BodyTypes[Idx]);
					if (Option == nullptr)
					{
						continue;
					}

					const bool IsSelected = (Option == Selected);
					if (ImGui::Selectable(Option, IsSelected))
					{
						SelectedIdx = Idx;
					}
				}

				ImGui::EndCombo();
			}

			ImGui::EndTable();
		}

		/************************
		 * Attributes.
		 ************************/
		{
			ImGui::BeginTable("##AttributesTable", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip);
			ImGui::TableSetupColumn("L", 0, ColWidth);
			ImGui::TableSetupColumn("V", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, ImGui::GetContentRegionAvail().x - ColWidth);

			ImGui::TableNextRow();
			static float GravityScale = 1.0f;
			UI::Draw::DragFloat("Gravity Scale", &GravityScale, 0.01f, 0.0f, 2.0f, "%.2f");

			ImGui::TableNextRow();
			static float Friction = 1.0f;
			UI::Draw::DragFloat("Friction", &Friction, 0.01f, 0.0f, 2.0f, "%.2f");

			ImGui::TableNextRow();
			static float Density = 1.0f;
			UI::Draw::DragFloat("Density", &Density, 0.01f, 0.0f, 1.0f, "%.2f");

			ImGui::TableNextRow();
			static glm::vec2 LinearVelocity = { 0.0f, 0.0f };
			UI::Draw::Vec2Control("Linear Velocity", LinearVelocity, 0.10f, 0.010f, 0.010f);

			ImGui::TableNextRow();
			static float AngularVelocity = 0.0f;
			UI::Draw::DragFloat("Angular Velocity", &AngularVelocity, 0.01f, 0.0f, 1.0f, "%.2f");

			ImGui::TableNextRow();
			static float LinearDamping = 0.0f;
			UI::Draw::DragFloat("Linear Damping", &LinearDamping, 0.01f, 0.0f, 1.0f, "%.2f");

			ImGui::TableNextRow();
			static float DirForce = 0.0f;
			UI::Draw::DragFloat("Directional Force", &DirForce, 0.01f, 0.0f, 1.0f, "%.2f");

			ImGui::TableNextRow();
			static float JumpImpulse = 0.0f;
			UI::Draw::DragFloat("Jump Impulse", &JumpImpulse, 0.01f, 0.0f, 1.0f, "%.2f");

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			UI::ShiftCursor(17.0f, 4.0f);
			ImGui::Text("Sensor");
			ImGui::TableSetColumnIndex(1);
			UI::ShiftCursorX(6);
			static bool bSensor = false;
			ImGui::Checkbox("##Sensor", &bSensor);

			ImGui::EndTable();
		}

		/************************
		 * Body Flags.
		 ************************/
		ImGui::Dummy(ImVec2(0, 1));
		ImGui::PushFont(UI::Font::Get(EFont::SourceSansPro, EFontSize::Large, EFontModifier::Bold));
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		const bool BodyFlagsOpened = ImGui::TreeNodeEx("Body Flags", ImGuiTreeNodeFlags_SpanAvailWidth);
		if (BodyFlagsOpened)
		{
			ImGui::PopFont();
			UI::FScopedStyle CellPadding(ImGuiStyleVar_CellPadding, ImVec2(0, 4));

			ImGui::BeginTable("##BodyFlagsTable", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip);
			ImGui::TableSetupColumn("L", 0, ColWidth);
			ImGui::TableSetupColumn("V", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, ImGui::GetContentRegionAvail().x - ColWidth);

			auto Label = [](std::string_view Str) -> void
			{
				ImGui::TableSetColumnIndex(0);
				UI::ShiftCursor(GAME_MENU_LABEL_INDENT_WIDTH, 4.0f);
				ImGui::Text(Str.data());
			};

			auto NextColumn = []() -> void
			{
				ImGui::TableSetColumnIndex(1);
				UI::ShiftCursorY(-2.0f);
			};

			/* PreSolveEvents. */
			ImGui::TableNextRow();
			{
				Label("Pre Solve Events");
				NextColumn();
				ImGui::Checkbox("##PreSolveEvents", &Data.BodyFlag.bPreSolveEvents);
			}

			/* Contact Events. */
			ImGui::TableNextRow();
			{
				Label("Contact Events");
				NextColumn();
				ImGui::Checkbox("##ContactEvents", &Data.BodyFlag.bContactEvents);
			}

			/* Sensor Events. */
			ImGui::TableNextRow();
			{
				Label("Sensor Events");
				NextColumn();
				ImGui::Checkbox("##SensorEvents", &Data.BodyFlag.bSensorEvents);
			}

			/* Bullet. */
			ImGui::TableNextRow();
			{
				Label("Bullet");
				NextColumn();
				ImGui::Checkbox("##Bullet", &Data.BodyFlag.bBullet);
			}

			ImGui::EndTable();
			ImGui::TreePop();
		}
		else
		{
			ImGui::PopFont();
		}

		/************************
		 * Motion Lock.
		 ************************/
		ImGui::PushFont(UI::Font::Get(EFont::SourceSansPro, EFontSize::Large, EFontModifier::Bold));
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		const bool MotionLockOpened = ImGui::TreeNodeEx("Motion Lock", ImGuiTreeNodeFlags_SpanAvailWidth);
		if (MotionLockOpened)
		{
			ImGui::PopFont();
			UI::ShiftCursorY(4.0f);
			UI::FScopedStyle CellPadding(ImGuiStyleVar_CellPadding, ImVec2(0, 4));

			/* Axis: X */
			if (ImGui::Checkbox("X", &Data.MotionLock.X))
			{
				if (Data.MotionLock.X)
				{
					Data.MotionLock.All = false;
				}
			}

			/* Axis: Y */
			ImGui::SameLine(0.0f, 18.0f);
			if (ImGui::Checkbox("Y", &Data.MotionLock.Y))
			{
				if (Data.MotionLock.Y)
				{
					Data.MotionLock.All = false;
				}
			}

			/* Axis: Z */
			ImGui::SameLine(0.0f, 18.0f);
			if (ImGui::Checkbox("Z", &Data.MotionLock.Z))
			{
				if (Data.MotionLock.Z)
				{
					Data.MotionLock.All = false;
				}
			}

			/* All */
			ImGui::SameLine(0.0f, 32.0f);
			if (ImGui::Checkbox("All", &Data.MotionLock.All))
			{
				if (Data.MotionLock.All)
				{
					Data.MotionLock.X = true;
					Data.MotionLock.Y = true;
					Data.MotionLock.Z = true;
				}
				else
				{
					Data.MotionLock.X = false;
					Data.MotionLock.Y = false;
					Data.MotionLock.Z = false;
				}
			}

			ImGui::TreePop();
		}
		else
		{
			ImGui::PopFont();
		}

		ImGui::PopID();

		Data.BodyType = static_cast<EBodyType>(SelectedIdx);
	}

}
