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
	FViewportData ViewportData;
	FActorAttributes ActorAttr;
	FPhysicsBodyData PhysicsBodyData;

	const std::array<const char*, std::to_underlying(ETexture::COUNT)> TextureNames = {
		Enum::ToString(ETexture::White),
		Enum::ToString(ETexture::Background),
		Enum::ToString(ETexture::Player),
		Enum::ToString(ETexture::Metal),
		Enum::ToString(ETexture::Bricks),
		Enum::ToString(ETexture::Wood),
		Enum::ToString(ETexture::Swoosh),
		Enum::ToString(ETexture::Cloud),
	};

	namespace {
		constexpr auto& ColorArray = FColor::GetArray();
	}

	const FViewportData& GetViewportData() { return ViewportData; }

	bool ColorDropdown(EColor& Selected)
	{
		bool Updated = false;
		std::size_t SelectedIdx = std::to_underlying(Selected);

		static const std::string Label = "Color";
		if (ImGui::GetCurrentTable() != nullptr)
		{
			ImGui::TableSetColumnIndex(0);
			UI::ShiftCursor(17.0f, 0.0f);
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
		Updated |= TextureDropdown(Attr.Texture);

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

			ActorCreateButtons(Scene);

			ImGui::TreePop();
		}
		else
		{
			UI::Font::Pop();
		}

		ImGui::PopID();
	}

	void ActorCreateButtons(std::shared_ptr<CScene> Scene)
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
						FColor::Get(ActorAttr.Color),
						ActorAttr.Texture
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

	bool DrawGizmo(const uint32_t Operation, CActor& Actor, const glm::mat4& ViewMatrix, const glm::mat4& ProjectionMatrix, const glm::vec3& CameraPos)
	{
		ImGuizmo::SetOrthographic(true);
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

		bool ShouldNotSnapValues = CKeyboard::IsKeyDown(EKey::LeftControl);
		float SnapValue = 0.010f;
		if (Operation == ImGuizmo::OPERATION::ROTATE)
		{
			SnapValue = 1.0f;
		}
		const float SnapValues[3] = { SnapValue, SnapValue, SnapValue };

		FTransformComponent& TC = Actor.GetTransformComponent();
		glm::mat4 TransformMatrix = TC.GetTransform();

		const bool Manipulated = ImGuizmo::Manipulate(
			glm::value_ptr(ViewMatrix),
			glm::value_ptr(ProjectionMatrix),
			static_cast<ImGuizmo::OPERATION>(Operation),
			ImGuizmo::WORLD,
			glm::value_ptr(TransformMatrix),
			nullptr,
			ShouldNotSnapValues ? nullptr : SnapValues
		);

		if (ImGuizmo::IsUsing())
		{
			glm::vec3 Translation;
			glm::vec3 Scale;
			glm::quat Rotation;
			Math::DecomposeTransform(TransformMatrix, Translation, Rotation, Scale);
			LK_UNUSED(Scale);

			if (Actor.GetPosition() != Translation)
			{
				Actor.SetPosition(Translation);
			}

			const float RotRad = Math::GetAngleRad(Rotation);
			if (Actor.GetRotation() != RotRad)
			{
				Actor.SetRotation(RotRad);
			}
		}

		return Manipulated;
	}

	void PlayerData(std::shared_ptr<CPlayer> Player)
	{
		LK_ASSERT(Player);
		CBody& PlayerBody = Player->GetBody();
		const FPlayerData& PlayerData = Player->GetData();
		FTransformComponent& TC = Player->GetTransformComponent();

		glm::vec2 PlayerBodyPos = Player->GetBody().GetPosition();
		ImGui::Text("Position: (%.2f, %.2f)", PlayerBodyPos.x, PlayerBodyPos.y);
		ImGui::Text("Movement State: %s", Enum::ToString(PlayerData.MovementState));
		ImGui::Text("Jump State: %s", PlayerData.bJumping ? "Jumping" : "On ground");

		auto [CurrentSpriteFrame, NextSpriteFrame] = Player->GetCurrentAndNextSpriteFrame();
		ImGui::Text("Current Sprite Frame: %d", CurrentSpriteFrame);
		ImGui::Text("Next Sprite Frame: %d", NextSpriteFrame);
		ImGui::Dummy(ImVec2(0, 4));

		const glm::vec2 PlayerSize = Player->GetSize();
		ImGui::Text("Size: (%.2f, %.2f)", PlayerSize.x, PlayerSize.y);
		ImGui::Text("TC Scale: (%.2f, %.2f)", TC.Scale.x, TC.Scale.y);

		CCamera& Camera = Player->GetCamera();
		ImGui::Text("Camera Zoom: %.2f", Camera.GetZoom());
		const bool CameraLocked = Player->IsCameraLocked();
		ImGui::Text("Camera Lock: %s", CameraLocked ? "Active" : "Not active");
		ImGui::SameLine();
		if (ImGui::Button("Toggle"))
		{
			Player->SetCameraLock(!CameraLocked);
		}

		const glm::vec2 LinearVelocity = PlayerBody.GetLinearVelocity();
		ImGui::Text("Linear Velocity: (%.2f, %.2f)", LinearVelocity.x, LinearVelocity.y);
		const float AngularVelocity = PlayerBody.GetAngularVelocity();
		ImGui::Text("Angular Velocity: %.2f", AngularVelocity);

		ImGui::PushItemWidth(160.0f);
		float PlayerJumpImpulse = Player->GetJumpImpulse();
		ImGui::SliderFloat("Jump Impulse", &PlayerJumpImpulse, 0.0f, 10.0f, "%.5f");
		if (ImGui::IsItemActive()) Player->SetJumpImpulse(PlayerJumpImpulse);

		float PlayerDirForce = Player->GetDirectionForce();
		ImGui::SliderFloat("Direction Force", &PlayerDirForce, 0.0f, 10.0f, "%.5f");
		if (ImGui::IsItemActive()) Player->SetDirectionForce(PlayerDirForce);
		ImGui::Text("Last Direction Force: %.5f", Player->GetLastDirectionForce());

		static float BodyScale = TC.Scale.x;
		ImGui::SliderFloat("Body Scale", &BodyScale, 0.0f, 2.0f, "%.2f");
		ImGui::SameLine();
		if (ImGui::Button("Apply##Scale"))
		{
			PlayerBody.SetScale(BodyScale);
		}

		float Mass = PlayerBody.GetMass();
		ImGui::SliderFloat("Mass", &Mass, 0.0f, 10.0f, "%.2f");
		if (ImGui::IsItemActive()) PlayerBody.SetMass(Mass);

		float PlayerFriction = PlayerBody.GetFriction();
		ImGui::SliderFloat("Friction", &PlayerFriction, 0.0f, 2.0f, "%.3f");
		if (ImGui::IsItemActive()) PlayerBody.SetFriction(PlayerFriction);

		float PlayerRestitution = PlayerBody.GetRestitution();
		ImGui::SliderFloat("Restitution", &PlayerRestitution, 0.0f, 2.0f, "%.3f");
		if (ImGui::IsItemActive()) PlayerBody.SetRestitution(PlayerRestitution);

		ImGui::PopItemWidth();

		ImGui::Dummy(ImVec2(0, 4));
		UI::Draw::ActorNode_Data(*Player);
		ImGui::Dummy(ImVec2(0, 4));
	}

	/** @todo: The initial position needs to be placed in an upper-left/right corner. */
	void Statistics()
	{
		CGameInstance* GameInstance = CGameInstance::Get();
		if (!GameInstance)
		{
			return;
		}

		/** @todo: Remove docknode tab */
		ImGui::SetNextWindowBgAlpha(0.25f);
		ImGui::SetNextWindowSize(ImVec2(284, 0), ImGuiCond_Always);
		static constexpr ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoDecoration;
		if (!ImGui::Begin("##Statistics", nullptr, WindowFlags))
		{
			ImGui::End();
			return;
		}

		CPlayer* Player = GameInstance->GetPlayer();
		CCamera& Camera = Player->GetCamera();

		static constexpr float LabelColumnWidth = 150.0f;
		ImGui::BeginTable("##VectorControl", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip);
		ImGui::TableSetupColumn("Label", 0, LabelColumnWidth);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, ImGui::GetContentRegionAvail().x - LabelColumnWidth);

		auto Label = [](std::string_view Str) -> void
		{
			ImGui::TableSetColumnIndex(0);
			UI::ShiftCursor(17.0f, 4.0f);
			ImGui::Text(Str.data());
		};

		auto NextColumn = []() -> void
		{
			ImGui::TableSetColumnIndex(1);
			UI::ShiftCursor(0.0f, 4.0f);
		};

		const float FPS = 1.0f / GameInstance->GetDeltaTime();
		/* FPS */
		ImGui::TableNextRow();
		Label("FPS");
		NextColumn();
		ImGui::Text("%1.f", FPS);

		/* Camera Zoom */
		ImGui::TableNextRow();
		Label("Camera Zoom");
		NextColumn();
		ImGui::Text("%.2f", Camera.GetZoom());

		/* Camera lock */
		ImGui::TableNextRow();
		Label("Camera Lock");
		NextColumn();
		const bool CameraLocked = Player->IsCameraLocked();
		ImGui::Text("%s", CameraLocked ? "Active" : "Not active");
		
		ImGui::EndTable();

		if (ImGui::IsWindowHovered())
		{
			const ImVec2 Pos = ImGui::GetWindowPos();
			const ImVec2 Size = ImGui::GetWindowSize();
			if (ImGui::BeginTooltip())
			{
				ImGui::Text("Position: (%.1f, %.1f)", Pos.x, Pos.y);
				ImGui::Text("Size: (%.1f, %.1f)", Size.x, Size.y);
				ImGui::EndTooltip();
			}
		}

		ImGui::End();
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
