#include "ui.h"

#include "core/window.h"
#include "core/input/keyboard.h"
#include "game/gameinstance.h"
#include "renderer/color.h"
#include "renderer/font.h"
#include "renderer/renderer.h"
#include "ui_core.h"
#include "widgets.h"
#include "game/spawner.h"
#include "scene/scene.h"

namespace platformer2d::UI {

	FOnGameMenuOpened OnGameMenuOpened;
	FActorAttributes ActorAttr;
	FPhysicsBodyData PhysicsBodyData;
	FViewportData ViewportData;

	namespace 
	{
		/* @todo: Use global config */
		constexpr float GAME_MENU_LABEL_COLUMN_WIDTH = 190.0f;
		constexpr float GAME_MENU_LABEL_INDENT_WIDTH = 24.0f;
		constexpr float GAME_MENU_COLUMN_ITEM_WIDTH = 410.0f;

		constexpr auto& ColorArray = FColor::GetArray();
		const std::array<const char*, CRenderer::MAX_TEXTURES> TextureNames = {
			Enum::ToString(ETexture::White),
			Enum::ToString(ETexture::Background),
			Enum::ToString(ETexture::Player),
			Enum::ToString(ETexture::Metal),
			Enum::ToString(ETexture::Bricks),
			Enum::ToString(ETexture::Wood),
			Enum::ToString(ETexture::Swoosh),
			Enum::ToString(ETexture::Cloud),
		};
	}

	const FViewportData& GetViewportData() { return ViewportData; }

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
		if (Data.BodyFlag.bBullet) BodyFlags |= EBodyFlag::EBodyFlag_IsBullet;
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

	bool ColorDropdown(EColor& Selected)
	{
		bool Updated = false;
		std::size_t SelectedIdx = std::to_underlying(Selected);

		static const std::string Label = "Color";
		if (ImGui::GetCurrentTable() != nullptr)
		{
			ImGui::TableSetColumnIndex(0);
			UI::ShiftCursor(17.0f, 7.0f);
			ImGui::Text(Label.c_str());

			ImGui::TableSetColumnIndex(1);
			UI::ShiftCursor(7.0f, 0.0f);
		}
		else
		{
			ImGui::Text(Label.c_str());
			ImGui::SameLine();
		}

		const float ComboItemWidth = ((ImGui::GetContentRegionAvail().x - 8.0f) / 2.0f);
		ImGui::SetNextItemWidth(ComboItemWidth);
		if (ImGui::BeginCombo("##Color", Enum::ToString(Selected)))
		{
			for (int Idx = 0; Idx < ColorArray.size(); Idx++)
			{
				const char* Option = Enum::ToString(ColorArray[Idx]);
				if (Option == nullptr)
				{
					continue;
				}

				const bool IsSelected = (SelectedIdx == Idx);
				if (ImGui::Selectable(Option, IsSelected))
				{
					SelectedIdx = Idx;
				}
			}

			ImGui::EndCombo();
			if (SelectedIdx != std::to_underlying(Selected))
			{
				Selected = static_cast<EColor>(SelectedIdx);
				Updated = true;
			}
		}

		return Updated;
	}

	bool ActorAttributes(FActorAttributes& Attr)
	{
		bool Updated = false;
#if 0 /* NO LABEL */
		UI::Font::Push(EFont::SourceSansPro, EFontSize::Header, EFontModifier::Bold);
		static constexpr const char* CreatorMenuLabel = "Creator";
		static const ImVec2 LabelSize = ImGui::CalcTextSize(CreatorMenuLabel);
		UI::ShiftCursorX((0.50f * Avail.x) - LabelSize.x);
		ImGui::Text("Creator Menu");
		UI::Font::Pop();
#endif
		static constexpr float ColWidth = 180.0f;

		ImGui::BeginTable("##ActorAttributes", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip);
		ImGui::TableSetupColumn("L", 0, ColWidth);
		ImGui::TableSetupColumn("V", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, ImGui::GetContentRegionAvail().x - ColWidth);

		/* Actor Name. */
		ImGui::TableNextRow();
		{
			ImGui::TableSetColumnIndex(0);
			UI::ShiftCursor(17.0f, 7.0f);
			ImGui::Text("Name");

			ImGui::TableSetColumnIndex(1);
			UI::ShiftCursor(7.0f, 0.0f);
			ImGui::SetNextItemWidth((ImGui::GetContentRegionAvail().x - 8.0f) / 2.0f);
			ImGui::InputText("##ActorName", Attr.NameBuf.data(), Attr.NameBuf.size());
		}

		ImGui::TableNextRow();
		Updated |= UI::Draw::Vec2Control("Position", Attr.Position, 0.0f, 0.010f, -100.0f, 100.0f);

		ImGui::TableNextRow();
		Updated |= UI::Draw::Vec2Control("Size", Attr.Size, 1.0f, 0.010f, 0.010f, 2.0f);

		ImGui::TableNextRow();
		Updated |= TextureDropDown(Attr.Texture);

		ImGui::TableNextRow();
		Updated |= ColorDropdown(Attr.Color);

		ImGui::EndTable();

		return Updated;
	}

	void CreatorMenu(std::shared_ptr<CScene> Scene)
	{
		if (!Scene)
		{
			return;
		}

		static const std::string FuncID = LK_FUNCSIG;
		ImGui::PushID(FuncID.c_str());

		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		UI::Font::Push(EFont::SourceSansPro, EFontSize::Header, EFontModifier::Bold);
		const bool CreateMenuOpened = ImGui::TreeNodeEx("Creator", ImGuiTreeNodeFlags_SpanAvailWidth);
		if (CreateMenuOpened)
		{
			UI::Font::Pop();
			const ImVec2 Avail = ImGui::GetContentRegionAvail();
			UI::FScopedStyle FramePadding(ImGuiStyleVar_FramePadding, ImVec2(6, 3));
			UI::FScopedStyle FrameRounding(ImGuiStyleVar_FrameRounding, 6.0f);

			ActorAttributes(ActorAttr);

			UI::ShiftCursorY(30);
			PhysicsBodyMenu(PhysicsBodyData);
			UI::ShiftCursorY(30);

			CreatorMenuButtons(Scene);

			ImGui::TreePop();
		}
		else
		{
			UI::Font::Pop();
		}

		ImGui::PopID();
	}

	void CreatorMenuButtons(std::shared_ptr<CScene> Scene)
	{
		LK_ASSERT(Scene);
		/* Button: Create */
		{
			static const ImVec2 Avail = ImGui::GetContentRegionAvail();
			static constexpr ImVec2 ButtonSize = ImVec2(112, 50);

			ImGui::BeginTable("##ActorButtons", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip);
			ImGui::TableSetupColumn("L", 0, Avail.x * 0.30f);
			ImGui::TableSetupColumn("V", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, Avail.x * 0.60);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(1);

			UI::FScopedFont Font(UI::Font::Get(EFont::SourceSansPro, EFontSize::Large, EFontModifier::Bold));
			UI::FScopedStyle ButtonFrame(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
			UI::FScopedStyle ButtonRounding(ImGuiStyleVar_FrameRounding, 8);
			{
				UI::FScopedColorStack ButtonColours(
					ImGuiCol_Button, RGBA32::LightGreen,
					ImGuiCol_ButtonHovered, RGBA32::DarkGreen,
					ImGuiCol_ButtonActive, RGBA32::NiceGreen
				);

				const bool ActorExists = Scene->DoesActorExist(ActorAttr.NameBuf.data());
				if (ActorExists)
				{
					ImGui::BeginDisabled();
				}

				UI::ShiftCursorX((0.50f * ImGui::GetContentRegionAvail().x) - (0.50f * ButtonSize.x));
				if (ImGui::Button("Create", ButtonSize))
				{
					FBodySpecification NewBodySpec;
					Aggregate(PhysicsBodyData, NewBodySpec);
					LK_INFO("{}", CBody::ToString(NewBodySpec));
					CSpawner::CreatePolygon(
						ActorAttr.NameBuf.data(),
						NewBodySpec,
						ActorAttr.Size,
						FColor::Get(ActorAttr.Color)
					);
				}
				if (ActorExists)
				{
					ImGui::EndDisabled();
				}
			}

			/* Button: Delete */
			{
				ImGui::SameLine();
				UI::FScopedColorStack ButtonColours(
					ImGuiCol_Button, RGBA32::WineRed,
					ImGuiCol_ButtonHovered, RGBA32::DarkRed,
					ImGuiCol_ButtonActive, RGBA32::Red
				);

				UI::ShiftCursorX((0.50f * ImGui::GetContentRegionAvail().x) - (0.50f * ButtonSize.x));
				if (ImGui::Button("Delete", ButtonSize))
				{
					LK_WARN("PLACEHOLDER");
				}
			}

			ImGui::EndTable();
		}
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

	void TextureModifier()
	{
		static constexpr float ButtonPaddingY = 7.0f;
		static constexpr ImVec2 ButtonSize(84, 42);
		static constexpr float ItemWidth = 2.0f * ButtonSize.x;

		ImGui::Dummy(ImVec2(0, 12));
		static ETexture SelectedTexture = ETexture::White;
		if (TextureDropDown(SelectedTexture))
		{
			LK_DEBUG("Selected: {}", Enum::ToString(SelectedTexture));
		}

		const ImVec2 Avail = ImGui::GetContentRegionAvail();
		if (const std::shared_ptr<CTexture> TextureRef = CRenderer::GetTexture(SelectedTexture); TextureRef != nullptr)
		{
			/* Texture preview. */
			ImGui::SameLine(0.0f, 12.0f);
			UI::ShiftCursorY(-4.0f);
			ImGui::Image(
				static_cast<ImU64>(TextureRef->GetID()),
				ImVec2(32.0f, 32.0f),
				ImVec2(0.0f, 1.0f), /* Uv0. */
				ImVec2(1.0f, 0.0f)  /* Uv1. */
			);

			static constexpr std::string_view Marker = "assets/textures/";
			auto StripPrefix = [](const std::filesystem::path& Path) -> std::string
			{
				const std::string Str = Path.generic_string();
				const std::size_t Pos = Str.find(Marker);
				if (Pos != std::string::npos)
				{
					return Str.substr(Pos);
				}
				return Str;
			};

			ImGui::Dummy(ImVec2(0, 6));

			{
				UI::FScopedFont Font(UI::Font::Get(EFont::SourceSansPro, EFontSize::Regular, EFontModifier::BoldItalic));
				ImGui::Indent();
				ImGui::Text("Size:%-4s%dx%d", " ", TextureRef->GetWidth(), TextureRef->GetHeight());
				const std::string TrimmedPath = StripPrefix(TextureRef->GetFilePath());
				ImGui::Text("Path:%-4s%s", " ", TrimmedPath.c_str());
				ImGui::Unindent();
			}
		}
		ImGui::Dummy(ImVec2(0, 12));

		{
			UI::FScopedStyle FrameRounding(ImGuiStyleVar_FrameRounding, 10);
			UI::FScopedStyle FramePadding(ImGuiStyleVar_FramePadding, ImVec2(8, 6));
			UI::FScopedStyle FrameBorder(ImGuiStyleVar_FrameBorderSize, 2.0f);
			UI::FScopedColor ButtonCol(ImGuiCol_Button, RGBA32::Titlebar::Default);
			UI::FScopedColor ButtonActiveCol(ImGuiCol_ButtonActive, RGBA32::LightGray);
			UI::FScopedColor ButtonHoveredCol(ImGuiCol_ButtonHovered, RGBA32::SelectionMuted);

			UI::ShiftCursorX(35.0f);
			ImGui::Text("Wrap");

			ImGui::SameLine((Avail.x * 0.50f) - (ItemWidth * 0.50f) + ButtonPaddingY);
			UI::ShiftCursorY(-ButtonPaddingY);
			if (ImGui::Button("Clamp", ButtonSize))
			{
				CRenderer::GetTexture(SelectedTexture)->SetWrap(ETextureWrap::Clamp);
			}
			ImGui::SameLine(0.0f, ButtonPaddingY);
			UI::ShiftCursorY(-ButtonPaddingY);
			if (ImGui::Button("Repeat", ButtonSize))
			{
				CRenderer::GetTexture(SelectedTexture)->SetWrap(ETextureWrap::Repeat);
			}

			ImGui::Dummy(ImVec2(0, 6));

			UI::ShiftCursorX(35.0f);
			ImGui::Text("Filter");

			ImGui::SameLine((Avail.x * 0.50f) - (ItemWidth * 0.50f) + ButtonPaddingY);
			UI::ShiftCursorY(-ButtonPaddingY);
			if (ImGui::Button("Linear", ButtonSize))
			{
				CRenderer::GetTexture(SelectedTexture)->SetFilter(ETextureFilter::Linear);
			}

			ImGui::SameLine(0.0f, ButtonPaddingY);
			UI::ShiftCursorY(-ButtonPaddingY);
			if (ImGui::Button("Nearest", ButtonSize))
			{
				CRenderer::GetTexture(SelectedTexture)->SetFilter(ETextureFilter::Nearest);
			}
		}
	}

	bool TextureDropDown(ETexture& Selected)
	{
		static constexpr float ButtonPaddingY = 7.0f;
		static constexpr ImVec2 ButtonSize(84, 42);
		static constexpr float ItemWidth = 2.0f * ButtonSize.x;

		bool Updated = false;
		std::size_t SelectedIdx = std::to_underlying(Selected);
		LK_ASSERT((SelectedIdx >= 0) && (SelectedIdx < TextureNames.size()));
		static const char* SelectedTexture = TextureNames[SelectedIdx];

		static const std::string Label = "Texture";
		if (ImGui::GetCurrentTable() != nullptr)
		{
			ImGui::TableSetColumnIndex(0);
			UI::ShiftCursor(17.0f, 7.0f);
			ImGui::Text(Label.c_str());

			ImGui::TableSetColumnIndex(1);
			UI::ShiftCursor(7.0f, 0.0f);
		}
		else
		{
			ImGui::Text(Label.c_str());
			ImGui::SameLine();
		}

		const float ComboItemWidth = ((ImGui::GetContentRegionAvail().x - 8.0f) / 2.0f);
		ImGui::SetNextItemWidth(ComboItemWidth);
		if (ImGui::BeginCombo("##Texture", TextureNames[SelectedIdx]))
		{
			for (int Idx = 0; Idx < TextureNames.size(); Idx++)
			{
				const char* Option = TextureNames[Idx];
				if (Option == nullptr)
				{
					continue;
				}

				const bool IsSelected = (SelectedIdx == Idx);
				if (ImGui::Selectable(Option, IsSelected))
				{
					SelectedIdx = Idx;
				}
			}

			if (SelectedIdx != std::to_underlying(Selected))
			{
				Selected = static_cast<ETexture>(SelectedIdx);
				Updated = true;
			}

			ImGui::EndCombo();
		}

		return Updated;
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
		ImGui::TableSetupColumn("LabelColumn", 0, GAME_MENU_LABEL_COLUMN_WIDTH);
		ImGui::TableSetupColumn("ValueColumn", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, ImGui::GetContentRegionAvail().x - GAME_MENU_LABEL_COLUMN_WIDTH);

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		UI::ShiftCursor(GAME_MENU_LABEL_INDENT_WIDTH, 4.0f);
		ImGui::Text("Source");

		ImGui::TableSetColumnIndex(1);
		UI::ShiftCursor(0.0f, 4.0f);
		ImGui::SetNextItemWidth(GAME_MENU_COLUMN_ITEM_WIDTH);
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
		UI::ShiftCursor(GAME_MENU_LABEL_INDENT_WIDTH, 4.0f);
		ImGui::Text("Destination");

		ImGui::TableSetColumnIndex(1);
		UI::ShiftCursor(0.0f, 4.0f);
		ImGui::SetNextItemWidth(GAME_MENU_COLUMN_ITEM_WIDTH);
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
		ImGui::TableSetupColumn("LabelColumn", 0, GAME_MENU_LABEL_COLUMN_WIDTH);
		ImGui::TableSetupColumn("ValueColumn", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, ImGui::GetContentRegionAvail().x - GAME_MENU_LABEL_COLUMN_WIDTH);

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		UI::ShiftCursor(GAME_MENU_LABEL_INDENT_WIDTH, 4.0f);
		ImGui::Text("Depth");

		ImGui::TableSetColumnIndex(1);
		UI::ShiftCursor(0.0f, 4.0f);
		ImGui::SetNextItemWidth(GAME_MENU_COLUMN_ITEM_WIDTH);
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

	void DrawGizmo(const uint32_t Operation, CActor& Actor, const glm::mat4& ViewMatrix, const glm::mat4& ProjectionMatrix, const glm::vec3& CameraPos)
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
			ImGuizmo::WORLD,
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
			LK_UNUSED(Scale);

			Actor.SetPosition(Translation);

			const float RotRad = Math::GetAngleRad(Rotation);
			if (Actor.GetRotation() != RotRad)
			{
				Actor.SetRotation(RotRad);
			}
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

			static const ImVec4 Colors[] = {
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

			const char Character[2] = { *Ptr, 0 };
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
			float Hue = std::fmod(Time + (*Ptr) * Speed, 1.0f);
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

	void PrepareLeftSidebar()
	{
		ImGuiWindow* SidebarWindow = ImGui::FindWindowByName(PanelID::Sidebar1);
		if (SidebarWindow == nullptr)
		{
			return;
		}

		ImGuiDockNode* DockNode = SidebarWindow->DockNode;
		if (DockNode == nullptr)
		{
			return;
		}

		if ((DockNode->Size.x <= 0.0f) || (DockNode->Size.y <= 0.0f))
		{
			return;
		}

		auto& V = ViewportData;
		ImGuiViewport* Viewport = ImGui::GetWindowViewport();

		V.LeftSidebarSize = { DockNode->Size.x, DockNode->Size.y };
		SidebarWindow->Pos = ImVec2(0, V.MenuBarSize.y);
		SidebarWindow->Size = ImVec2(V.LeftSidebarSize.x, Viewport->WorkSize.y);

		DockNode->LocalFlags &= ~ImGuiDockNodeFlags_NoDockingSplit;

		/* Dock node has no other windows docked in it. */
		if (DockNode->Windows.Size <= 1)
		{
			DockNode->LocalFlags |= ImGuiDockNodeFlags_NoWindowMenuButton;
			DockNode->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
			SidebarWindow->Flags &= ~ImGuiWindowFlags_NoTitleBar;
			if (DockNode->VisibleWindow)
			{
				DockNode->VisibleWindow->Flags &= ~ImGuiWindowFlags_NoTitleBar;
			}
		}
		else if (DockNode->Windows.Size > 1)
		{
			DockNode->LocalFlags &= ~ImGuiDockNodeFlags_NoTabBar;
			DockNode->LocalFlags &= ~ImGuiDockNodeFlags_NoWindowMenuButton;

			if (DockNode->VisibleWindow)
			{
				DockNode->VisibleWindow->Flags &= ~ImGuiWindowFlags_NoTitleBar;
			}
		}
	}

	void PrepareRightSidebar()
	{
		ImGuiWindow* SidebarWindow = ImGui::FindWindowByName(PanelID::Sidebar2);
		if (SidebarWindow == nullptr)
		{
			return;
		}

		ImGuiDockNode* DockNode = SidebarWindow->DockNode;
		if (DockNode == nullptr)
		{
			return;
		}

		if ((DockNode->Size.x <= 0.0f) || (DockNode->Size.y <= 0.0f))
		{
			return;
		}

		auto& V = ViewportData;
		ImGuiViewport* Viewport = ImGui::GetWindowViewport();

		V.RightSidebarSize = { DockNode->Size.x, DockNode->Size.y };
		SidebarWindow->Pos = ImVec2(Viewport->Size.x - V.RightSidebarSize.x, V.MenuBarSize.y);
		SidebarWindow->Size = ImVec2(V.RightSidebarSize.x, V.RightSidebarSize.y);

		DockNode->LocalFlags &= ~ImGuiDockNodeFlags_NoDocking;
		DockNode->LocalFlags &= ~ImGuiDockNodeFlags_NoDockingSplit;

		/* Dock node has no other windows docked in it. */
		if (DockNode->Windows.Size <= 1)
		{
			DockNode->LocalFlags |= ImGuiDockNodeFlags_NoWindowMenuButton;
			DockNode->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
			SidebarWindow->Flags &= ~ImGuiWindowFlags_NoTitleBar;

			if (DockNode->VisibleWindow)
			{
				DockNode->VisibleWindow->Flags &= ~ImGuiWindowFlags_NoTitleBar;
			}
		}
		else if (DockNode->Windows.Size > 1)
		{
			DockNode->LocalFlags &= ~ImGuiDockNodeFlags_NoTabBar;
			SidebarWindow->Flags &= ~ImGuiWindowFlags_NoTitleBar;

			if (DockNode->VisibleWindow)
			{
				DockNode->VisibleWindow->Flags &= ~ImGuiWindowFlags_NoTitleBar;
			}
		}
	}

}
