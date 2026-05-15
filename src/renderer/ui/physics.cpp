#include "ui.h"

#include "core/window.h"
#include "core/input/keyboard.h"
#include "game/instance.h"
#include "renderer/color.h"
#include "renderer/font.h"
#include "renderer/renderer.h"
#include "ui_core.h"
#include "combo.h"
#include "widgets.h"
#include "scene/scene.h"

namespace platformer2d::UI {

	void Aggregate(const FPhysicsBodyData& Data, FBodySpecification& BodySpec)
	{
		BodySpec.Type = Data.BodyType;
		BodySpec.Position = {Data.Position.x, Data.Position.y};
		BodySpec.Friction = Data.Friction;
		BodySpec.Density = Data.Density;
		BodySpec.GravityScale = Data.GravityScale;
		BodySpec.LinearVelocity = Data.LinearVelocity;
		BodySpec.LinearDamping = Data.LinearDamping;
		BodySpec.AngularVelocity = Data.AngularVelocity;
		BodySpec.AngularDamping = Data.AngularDamping;
		BodySpec.DirForce = Data.DirForce;
		BodySpec.JumpImpulse = Data.JumpImpulse;
		BodySpec.bSensor = Data.bSensor;

		/* Body flags. */
		int BodyFlags = EBodyFlag::EBodyFlag_None;
		if (Data.BodyFlag.bPreSolveEvents) {
			BodyFlags |= EBodyFlag::EBodyFlag_PreSolveEvents;
		}
		if (Data.BodyFlag.bContactEvents) {
			BodyFlags |= EBodyFlag::EBodyFlag_ContactEvents;
		}
		if (Data.BodyFlag.bSensorEvents) {
			BodyFlags |= EBodyFlag::EBodyFlag_SensorEvents;
		}
		if (Data.BodyFlag.bBullet) {
			BodyFlags |= EBodyFlag::EBodyFlag_Bullet;
		}
		BodySpec.Flags = static_cast<EBodyFlag>(BodyFlags);

		/* Motion lock flags. */
		int MotionLockFlags = EMotionLock::EMotionLock_None;
		if (Data.MotionLock.All) {
			MotionLockFlags = EMotionLock::EMotionLock_All;
		} else {
			if (Data.MotionLock.X) {
				MotionLockFlags |= std::to_underlying(EMotionLock::EMotionLock_X);
			}
			if (Data.MotionLock.Y) {
				MotionLockFlags |= std::to_underlying(EMotionLock::EMotionLock_Y);
			}
			if (Data.MotionLock.Z) {
				MotionLockFlags |= std::to_underlying(EMotionLock::EMotionLock_Z);
			}
		}
		BodySpec.MotionLock = static_cast<EMotionLock>(MotionLockFlags);
	}

	void PhysicsBodyMenu(FPhysicsBodyData& Data)
	{
		constexpr float ButtonPaddingY = 7.0f;
		constexpr ImVec2 ButtonSize(84, 42);
		constexpr float ItemWidth = 2.0f * ButtonSize.x;
		constexpr float ColWidth = 180.0f;

		ImGui::PushID("PhysicsBodyMenu");
		static EBodyType Selected = EBodyType::Static;

		const ImVec2 Avail = ImGui::GetContentRegionAvail();

		/************************
		 * Body Type.
		 ************************/
		{
			/* @todo: Use UI::Table here instead. */
			ImGui::BeginTable("##BodyTypeTable", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip);
			ImGui::TableSetupColumn("L", 0, ColWidth);
			ImGui::TableSetupColumn("V", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, ImGui::GetContentRegionAvail().x - ColWidth);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			UI::ShiftCursor(17.0f, 7.0f);
			ImGui::Text("Body Type");

			ImGui::TableSetColumnIndex(1);
			UI::ShiftCursor(7.0f, 0.0f);
			if (UI::Combo("##BodyType", Enum::View<EBodyType>(), Selected)) {
				LK_TRACE_TAG("UI", "Combo -> BodyType: {}", Enum::ToString(Selected));
				Data.BodyType = Selected;
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
			UI::DragFloat("Gravity Scale", Data.GravityScale, 0.01f, 0.0f, 2.0f, "%.2f");

			ImGui::TableNextRow();
			UI::DragFloat("Friction", Data.Friction, 0.01f, 0.0f, 2.0f, "%.2f");

			ImGui::TableNextRow();
			UI::DragFloat("Density", Data.Density, 0.01f, 0.0f, 1.0f, "%.2f");

			ImGui::TableNextRow();
			UI::DragFloat2("Linear Velocity", Data.LinearVelocity, 0.10f, 0.010f, 0.010f);

			ImGui::TableNextRow();
			UI::DragFloat("Angular Velocity", Data.AngularVelocity, 0.01f, 0.0f, 1.0f, "%.2f");

			ImGui::TableNextRow();
			UI::DragFloat("Linear Damping", Data.LinearDamping, 0.01f, 0.0f, 1.0f, "%.2f");

			ImGui::TableNextRow();
			UI::DragFloat("Directional Force", Data.DirForce, 0.01f, 0.0f, 1.0f, "%.2f");

			ImGui::TableNextRow();
			UI::DragFloat("Jump Impulse", Data.JumpImpulse, 0.01f, 0.0f, 1.0f, "%.2f");

			ImGui::TableNextRow();
			UI::Checkbox("Sensor", Data.bSensor);

			ImGui::EndTable();
		}

		/************************
		 * Body Flags.
		 ************************/
		ImGui::Dummy(ImVec2(0, 1));
		ImGui::PushFont(UI::Font::Get(EFont::SourceSansPro, EFontSize::Large, EFontModifier::Bold));
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		const bool BodyFlagsOpened = ImGui::TreeNodeEx("Body Flags", ImGuiTreeNodeFlags_SpanAvailWidth);
		if (BodyFlagsOpened) {
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
		} else {
			ImGui::PopFont();
		}

		/************************
		 * Motion Lock.
		 ************************/
		ImGui::PushFont(UI::Font::Get(EFont::SourceSansPro, EFontSize::Large, EFontModifier::Bold));
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		const bool MotionLockOpened = ImGui::TreeNodeEx("Motion Lock", ImGuiTreeNodeFlags_SpanAvailWidth);
		if (MotionLockOpened) {
			ImGui::PopFont();
			UI::ShiftCursorY(4.0f);
			UI::FScopedStyle CellPadding(ImGuiStyleVar_CellPadding, ImVec2(0, 4));

			/* Axis: X */
			if (ImGui::Checkbox("X", &Data.MotionLock.X)) {
				if (Data.MotionLock.X) {
					Data.MotionLock.All = false;
				}
			}

			/* Axis: Y */
			ImGui::SameLine(0.0f, 18.0f);
			if (ImGui::Checkbox("Y", &Data.MotionLock.Y)) {
				if (Data.MotionLock.Y) {
					Data.MotionLock.All = false;
				}
			}

			/* Axis: Z */
			ImGui::SameLine(0.0f, 18.0f);
			if (ImGui::Checkbox("Z", &Data.MotionLock.Z)) {
				if (Data.MotionLock.Z) {
					Data.MotionLock.All = false;
				}
			}

			/* All */
			ImGui::SameLine(0.0f, 32.0f);
			if (ImGui::Checkbox("All", &Data.MotionLock.All)) {
				if (Data.MotionLock.All) {
					Data.MotionLock.X = true;
					Data.MotionLock.Y = true;
					Data.MotionLock.Z = true;
				} else {
					Data.MotionLock.X = false;
					Data.MotionLock.Y = false;
					Data.MotionLock.Z = false;
				}
			}

			ImGui::TreePop();
		} else {
			ImGui::PopFont();
		}

		ImGui::PopID();
	}

}
