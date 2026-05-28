#include "physics.h"

#include "core/window.h"
#include "core/input/keyboard.h"
#include "game/instance.h"
#include "renderer/color.h"
#include "renderer/font.h"
#include "renderer/renderer.h"
#include "ui.h"
#include "ui_core.h"
#include "combo.h"
#include "widgets.h"
#include "scene/scene.h"

namespace platformer2d::UI {

	FPhysicsBodyData PhysicsBodyData;

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
		std::underlying_type_t<EBodyFlag> BodyFlags = EBodyFlag::EBodyFlag_None;
		if (Data.BodyFlag.bPreSolveEvents) {
			LK_DEBUG_TAG("UI", "BodyFlag: PreSolve");
			BodyFlags |= EBodyFlag::EBodyFlag_PreSolveEvents;
		}
		if (Data.BodyFlag.bContactEvents) {
			LK_DEBUG_TAG("UI", "BodyFlag: Contact");
			BodyFlags |= EBodyFlag::EBodyFlag_ContactEvents;
		}
		if (Data.BodyFlag.bSensorEvents) {
			LK_DEBUG_TAG("UI", "BodyFlag: Sensor");
			BodyFlags |= EBodyFlag::EBodyFlag_SensorEvents;
		}
		if (Data.BodyFlag.bBullet) {
			LK_DEBUG_TAG("UI", "BodyFlag: Bullet");
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
		BeginPropertyGrid();
		{
			Table::NextRow();
			ImGui::Text("Body Type");

			Table::NextColumn();
			if (UI::Combo("##BodyType", Enum::View<EBodyType>(), Selected)) {
				LK_TRACE_TAG("UI", "BodyType: {}", Enum::ToString(Selected));
				Data.BodyType = Selected;
			}
		}
		EndPropertyGrid();

		/************************
		 * Attributes.
		 ************************/
		if (ImGui::TreeNodeEx("Physics", ImGuiTreeNodeFlags_SpanAvailWidth)) {
			BeginPropertyGrid();
			Table::NextRow();
			UI::DragFloat("Gravity Scale", Data.GravityScale, 0.01f, 0.0f, 2.0f, "%.2f");

			Table::NextRow();
			UI::DragFloat("Friction", Data.Friction, 0.01f, 0.0f, 2.0f, "%.2f");

			Table::NextRow();
			UI::DragFloat("Density", Data.Density, 0.01f, 0.0f, 1.0f, "%.2f");

			Table::NextRow();
			UI::DragFloat2("Linear Velocity", Data.LinearVelocity, 0.10f, 0.010f, 0.010f);

			Table::NextRow();
			UI::DragFloat("Angular Velocity", Data.AngularVelocity, 0.01f, 0.0f, 1.0f, "%.2f");

			Table::NextRow();
			UI::DragFloat("Linear Damping", Data.LinearDamping, 0.01f, 0.0f, 1.0f, "%.2f");

			Table::NextRow();
			UI::DragFloat("Directional Force", Data.DirForce, 0.01f, 0.0f, 1.0f, "%.2f");

			Table::NextRow();
			UI::DragFloat("Jump Impulse", Data.JumpImpulse, 0.01f, 0.0f, 1.0f, "%.2f");

			EndPropertyGrid();
			ImGui::TreePop();
		}

		BeginPropertyGrid();
		{
			Table::NextRow();
			UI::Checkbox("Sensor", Data.bSensor);
		}
		EndPropertyGrid();

		/************************
		 * Body Flags.
		 ************************/
		ImGui::Dummy(ImVec2(0, 1));
		ImGui::PushFont(UI::Font::Get(EFont::SourceSansPro, EFontSize::Large, EFontModifier::Bold));
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		const bool BodyFlagsOpened = ImGui::TreeNodeEx("Body Flags", ImGuiTreeNodeFlags_SpanAvailWidth);
		ImGui::PopFont();
		if (BodyFlagsOpened) {
			ImGui::Dummy(ImVec2(0, 2));
			UI::ShiftCursorX(4.0f);

			FChipRow Row;
			const auto Chip = [&](const char* Label, bool& Value)
			{
				Row.Next(Label);
				if (UI::FlagChip(Label, Value, RGBA32::Orange)) {
					Value = !Value;
				}
			};
			Chip("PreSolve", Data.BodyFlag.bPreSolveEvents);
			Chip("Contact", Data.BodyFlag.bContactEvents);
			Chip("Sensor", Data.BodyFlag.bSensorEvents);
			Chip("Bullet", Data.BodyFlag.bBullet);

			ImGui::TreePop();
		}

		/************************
		 * Motion Lock.
		 ************************/
		UI::Font::Push(EFont::SourceSansPro, EFontSize::Large, EFontModifier::Bold);
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		const bool MotionLockOpened = ImGui::TreeNodeEx("Motion Lock", ImGuiTreeNodeFlags_SpanAvailWidth);
		UI::Font::Pop();
		if (MotionLockOpened) {
			ImGui::Dummy(ImVec2(0, 2));
			UI::ShiftCursorX(4.0f);

			FChipRow Row;
			const auto AxisChip = [&](const char* Label, bool& Value)
			{
				Row.Next(Label);
				if (UI::FlagChip(Label, Value, RGBA32::NiceBlue)) {
					Value = !Value;
					if (Value) {
						Data.MotionLock.All = false;
					}
				}
			};
			AxisChip("X", Data.MotionLock.X);
			AxisChip("Y", Data.MotionLock.Y);
			AxisChip("Z", Data.MotionLock.Z);

			Row.Next("All");
			if (UI::FlagChip("All", Data.MotionLock.All, RGBA32::NiceGreen)) {
				Data.MotionLock.All = !Data.MotionLock.All;
				Data.MotionLock.X = Data.MotionLock.All;
				Data.MotionLock.Y = Data.MotionLock.All;
				Data.MotionLock.Z = Data.MotionLock.All;
			}

			ImGui::TreePop();
		}

		ImGui::PopID();
	}

}
