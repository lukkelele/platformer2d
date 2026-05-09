#include "ui.h"

#include "core/window.h"
#include "core/input/keyboard.h"
#include "core/selectioncontext.h"
#include "game/instance.h"
#include "game/gameplaysystem.h"
#include "renderer/color.h"
#include "renderer/debugrenderer.h"
#include "renderer/font.h"
#include "renderer/renderer.h"
#include "ui_core.h"
#include "widgets.h"
#include "game/spawner.h"
#include "game/rifle.h"
#include "scene/scene.h"

namespace platformer2d::UI {
	FOnPauseMenuOpened OnPauseMenuOpened;
	FViewportData ViewportData;
	FActorAttributes ActorAttr;
	FPhysicsBodyData PhysicsBodyData;

	namespace {
		constexpr auto& ColorArray = FColor::GetArray();
	}

	const FViewportData& GetViewportData()
	{
		return ViewportData;
	}

	bool BeginPropertyGrid(const std::size_t LabelColumnWidth)
	{
		UI::PushID();
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 0));
		if (ImGui::BeginTable("##PropertyGrid", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip)) {
			ImGui::TableSetupColumn("L", 0, LabelColumnWidth);
			ImGui::TableSetupColumn("V", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, ImGui::GetContentRegionAvail().x - LabelColumnWidth);
			return true;
		} else {
			ImGui::PopStyleVar(1);
			UI::PopID();
			return false;
		}
	}

	void EndPropertyGrid()
	{
		ImGui::EndTable();
		ImGui::PopStyleVar(1); /* ItemSpacing */
		UI::PopID();
	}

	bool ColorDropdown(EColor& Selected)
	{
		bool Updated = false;
		std::size_t SelectedIdx = std::to_underlying(Selected);

		static const std::string Label = "Color";
		if (ImGui::GetCurrentTable() != nullptr) {
			ImGui::TableSetColumnIndex(0);
			UI::ShiftCursor(17.0f, 0.0f);
			ImGui::Text(Label.c_str());

			ImGui::TableSetColumnIndex(1);
			UI::ShiftCursor(7.0f, 0.0f);
		} else {
			ImGui::Text(Label.c_str());
			ImGui::SameLine();
		}

		const float ComboItemWidth = ((ImGui::GetContentRegionAvail().x - 8.0f) / 2.0f);
		ImGui::SetNextItemWidth(ComboItemWidth);
		if (ImGui::BeginCombo("##Color", Enum::ToString(Selected))) {
			for (int Idx = 0; Idx < ColorArray.size(); Idx++) {
				const char* Option = Enum::ToString(ColorArray[Idx]);
				if (Option == nullptr) {
					continue;
				}

				const bool IsSelected = (SelectedIdx == Idx);
				if (ImGui::Selectable(Option, IsSelected)) {
					SelectedIdx = Idx;
				}
			}
			ImGui::EndCombo();

			if (SelectedIdx != std::to_underlying(Selected)) {
				Selected = static_cast<EColor>(SelectedIdx);
				Updated = true;
			}
		}

		return Updated;
	}

	bool ActorAttributes(FActorAttributes& Attr)
	{
		bool Updated = false;
		static constexpr float ColWidth = 180.0f;

		ImGui::BeginTable("##ActorAttributes", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip);
		ImGui::TableSetupColumn("L", 0, ColWidth);
		ImGui::TableSetupColumn("V", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, ImGui::GetContentRegionAvail().x - ColWidth);

		/* Actor Name. */
		ImGui::TableNextRow();
		{
			UI::Table::Label("Name");
			UI::Table::NextColumn();
			ImGui::SetNextItemWidth((ImGui::GetContentRegionAvail().x - 8.0f) / 2.0f);
			ImGui::InputText("##ActorName", Attr.NameBuf.data(), Attr.NameBuf.size());
		}

		ImGui::TableNextRow();
		Updated |= UI::Widget::DragFloat2("Position", Attr.Position, 0.0f, 0.010f, -100.0f, 100.0f);

		ImGui::TableNextRow();
		Updated |= UI::Widget::DragFloat2("Size", Attr.Size, 1.0f, 0.010f, 0.010f, 2.0f);

		ImGui::TableNextRow();
		Updated |= UI::Widget::Combo::TextureDropdown(Attr.Texture);

		ImGui::TableNextRow();
		Updated |= ColorDropdown(Attr.Color);

		ImGui::EndTable();

		return Updated;
	}

	void CreatorMenu(std::shared_ptr<CScene> Scene)
	{
		const bool WindowOpened = UI::Begin(PanelID::CreatorMenu, nullptr);
		if (!WindowOpened) {
			return;
		}
		if (!Scene) {
			UI::End();
			return;
		}

		static const std::string FuncID = LK_FUNCSIG;
		ImGui::PushID(FuncID.c_str());

		{
			UI::FScopedStyle FramePadding(ImGuiStyleVar_FramePadding, ImVec2(6, 3));
			UI::FScopedStyle FrameRounding(ImGuiStyleVar_FrameRounding, 6.0f);
			static constexpr ImVec2 ButtonSize = ImVec2(142, 50);
			if (ImGui::Button("Add Spawnpoint", ButtonSize)) {
				LK_WARN("Add spawnpoint");
				CSpawner::CreateSpawnpoint("PlayerSpawn", {0.0f, 0.0f});
			}

			ImGui::SameLine();
			if (ImGui::Button("Teleport Player", ButtonSize)) {
				auto Player = CGameInstance::Get()->GetPlayer(0);
				CGameplaySystem::Teleport(Player, {0.0f, 0.0f});
			}

#if 0 /* @todo Should be replaced by chain segments */
			ImGui::PushStyleColor(ImGuiCol_Button, RGBA32::Orange);
			if (ImGui::Button("Template: Floor", {ButtonSize.x + 64, ButtonSize.y})) {
				auto Player = CGameInstance::Get()->GetPlayer(0);
				ActorAttr.Size = glm::vec2(1.46, 0.11);
				const glm::vec3 PlayerPos = Player->GetPosition();
				ActorAttr.Position = {PlayerPos.x, PlayerPos.y + 0.15f};
				PhysicsBodyData.Position = glm::vec3(ActorAttr.Position, 0.0f);
			}
			ImGui::PopStyleColor(1);

			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Button, RGBA32::Orange);
			if (ImGui::Button("Template: Large Floor", {ButtonSize.x + 64, ButtonSize.y})) {
				auto Player = CGameInstance::Get()->GetPlayer(0);
				ActorAttr.Size = glm::vec2(2.24, 0.11);
				const glm::vec3 PlayerPos = Player->GetPosition();
				ActorAttr.Position = {PlayerPos.x, PlayerPos.y + 0.15f};
			}
			ImGui::PopStyleColor(1);
#endif
		}

		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		UI::Font::Push(EFont::SourceSansPro, EFontSize::Header, EFontModifier::Bold);
		const bool CreateMenuOpened = ImGui::TreeNodeEx("Creator", ImGuiTreeNodeFlags_SpanAvailWidth);
		UI::Font::Pop();
		if (CreateMenuOpened) {
			const ImVec2 Avail = ImGui::GetContentRegionAvail();
			UI::FScopedStyle FramePadding(ImGuiStyleVar_FramePadding, ImVec2(6, 3));
			UI::FScopedStyle FrameRounding(ImGuiStyleVar_FrameRounding, 6.0f);

			ActorAttributes(ActorAttr);

			/* Menu: Physics body */
			UI::ShiftCursorY(20);
			PhysicsBodyMenu(PhysicsBodyData);
			UI::ShiftCursorY(20);

			ActorCreateButtons(Scene);

			ImGui::TreePop();
		}

		ImGui::PopID();
		UI::End();
	}

	void ActorCreateButtons(std::shared_ptr<CScene> Scene)
	{
		LK_ASSERT(Scene);
		static const ImVec2 Avail = ImGui::GetContentRegionAvail();
		static constexpr ImVec2 ButtonSize = ImVec2(112, 50);

		ImGui::BeginTable("##ActorButtons", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip);
		ImGui::TableSetupColumn("L", 0, Avail.x * 0.30f);
		ImGui::TableSetupColumn("V", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, Avail.x * 0.60);

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(1);

		/* Button: Create */
		UI::FScopedFont Font(UI::Font::Get(EFont::SourceSansPro, EFontSize::Large, EFontModifier::Bold));
		UI::FScopedStyle ButtonFrame(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
		UI::FScopedStyle ButtonRounding(ImGuiStyleVar_FrameRounding, 8);
		{
			UI::FScopedColorStack ButtonColours(
				ImGuiCol_Button, RGBA32::LightGreen,
				ImGuiCol_ButtonHovered, RGBA32::DarkGreen,
				ImGuiCol_ButtonActive, RGBA32::NiceGreen);

			const bool ActorExists = Scene->DoesActorExist(ActorAttr.NameBuf.data());
			if (ActorExists) {
				ImGui::BeginDisabled();
			}

			UI::ShiftCursorX((0.50f * ImGui::GetContentRegionAvail().x) - (0.50f * ButtonSize.x));
			if (ImGui::Button("Create", ButtonSize)) {
				FBodySpecification NewBodySpec;
				Aggregate(PhysicsBodyData, NewBodySpec);
				LK_INFO("{}", CBody::ToString(NewBodySpec));

				if (NewBodySpec.Type == EBodyType::Static) {
					std::shared_ptr<CActor> SpawnedPolygon = CSpawner::CreateStaticPolygon(
						ActorAttr.NameBuf.data(),
						ActorAttr.Position,
						ActorAttr.Size,
						FColor::Get(ActorAttr.Color));

					auto Player = CGameInstance::Get()->GetPlayer(0);
					const glm::vec3 PlayerPos = Player->GetPosition();
					CGameplaySystem::Teleport(SpawnedPolygon, {PlayerPos.x, PlayerPos.y + 0.50f});
				} else if (NewBodySpec.Type == EBodyType::Dynamic) {
					std::shared_ptr<CActor> SpawnedPolygon = CSpawner::CreatePolygon(
						ActorAttr.NameBuf.data(),
						NewBodySpec,
						ActorAttr.Size,
						FColor::Get(ActorAttr.Color),
						ActorAttr.Texture);

					auto Player = CGameInstance::Get()->GetPlayer(0);
					const glm::vec3 PlayerPos = Player->GetPosition();
					CGameplaySystem::Teleport(SpawnedPolygon, {PlayerPos.x, PlayerPos.y + 0.50f});
				}
			}

			if (ActorExists) {
				ImGui::EndDisabled();
			}
		}

		ImGui::EndTable();
	}

	bool DrawGizmo(const uint32_t Operation, CActor& Actor, const glm::mat4& ViewMatrix, const glm::mat4& ProjectionMatrix, const glm::vec3& CameraPos)
	{
		ImGuizmo::SetOrthographic(true);
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

		bool ShouldNotSnapValues = CKeyboard::IsKeyDown(EKey::LeftControl);
		float SnapValue = 0.010f;
		if (Operation == ImGuizmo::OPERATION::ROTATE) {
			SnapValue = 1.0f;
		}
		const float SnapValues[3] = {SnapValue, SnapValue, SnapValue};

		FTransformComponent& TC = Actor.GetTransformComponent();
		glm::mat4 TransformMatrix = TC.GetTransform();

		const bool Manipulated = ImGuizmo::Manipulate(
			glm::value_ptr(ViewMatrix),
			glm::value_ptr(ProjectionMatrix),
			static_cast<ImGuizmo::OPERATION>(Operation),
			ImGuizmo::WORLD,
			glm::value_ptr(TransformMatrix),
			nullptr,
			ShouldNotSnapValues ? nullptr : SnapValues);

		const bool IsUsing = ImGuizmo::IsUsing();
		if (IsUsing) {
			glm::vec3 Translation;
			glm::vec3 Scale;
			glm::quat Rotation;
			Math::DecomposeTransform(TransformMatrix, Translation, Rotation, Scale);

			if (Actor.GetPosition() != Translation) {
				Actor.SetPosition(Translation);
			}

			const float RotRad = Math::GetAngleRad(Rotation);
			if (Actor.GetRotation() != RotRad) {
				Actor.SetRotation(RotRad);
			}

#if 0 /* SCALING NEEDS TO BE SUPPORTED */
			if (TC.GetScale() != Scale) {
				if (CBody* Body = Actor.GetBody()) {
					glm::vec2 BodySize = Body->GetSize();
					FPolygon& Shape = Body->GetShape<EShape::Polygon>();
					BodySize.x *= Scale.x;
					BodySize.y *= Scale.y;
					Shape.Size = BodySize;
					TC.Scale = Scale;
				}
			}
#endif
		}

		return Manipulated;
	}

	void PlayerData(std::shared_ptr<CPlayer> Player)
	{
		LK_ASSERT(Player);
		CCamera& Camera = Player->GetCamera();
		CBody* Body = Player->GetBody();
		const FPlayerData& Data = Player->GetData();
		FTransformComponent& TC = Player->GetTransformComponent();

		static constexpr float LabelColumnWidth = 200.0f;
		ImGui::BeginTable("##PlayerDataTable", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip);
		ImGui::TableSetupColumn("Label", 0, LabelColumnWidth);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, ImGui::GetContentRegionAvail().x - LabelColumnWidth);

		bool Changed = false;

		auto Label = [](std::string_view Str) -> void
		{
			ImGui::TableSetColumnIndex(0);
			UI::ShiftCursor(17.0f, 4.0f);
			ImGui::Text(Str.data());
		};

		auto NextColumn = []() -> void
		{
			ImGui::TableSetColumnIndex(1);
			UI::ShiftCursor(9.0f, 2.0f);
		};

		ImGui::TableNextRow();
		glm::vec2 BodyPos = Body->GetPosition();
		{
			Label("Position");
			NextColumn();
			ImGui::Text("(%.2f, %.2f)", BodyPos.x, BodyPos.y);
		}

		/* Movement state. */
		ImGui::TableNextRow();
		{
			Label("Movement State");
			NextColumn();
			ImGui::Text("%s", Enum::ToString(Data.MovementState));
		}

		/* Jump state. */
		ImGui::TableNextRow();
		{
			Label("Jump State");
			NextColumn();
			ImGui::Text("%s", Data.bJumping ? "Jumping" : "On ground");
		}

		auto [CurrentSpriteFrame, NextSpriteFrame] = Player->GetCurrentAndNextSpriteFrame();
		ImGui::TableNextRow();
		{
			Label("Current Sprite Frame");
			NextColumn();
			ImGui::Text("(%u, %u)", CurrentSpriteFrame.X, CurrentSpriteFrame.Y);
		}
		ImGui::TableNextRow();
		{
			Label("Next Sprite Frame");
			NextColumn();
			ImGui::Text("(%u, %u)", NextSpriteFrame.X, NextSpriteFrame.Y);
		}

		/* Size. */
		ImGui::TableNextRow();
		{
			const glm::vec2 PlayerSize = Player->GetSize();
			Label("Size");
			NextColumn();
			ImGui::Text("(%.2f, %.2f)", PlayerSize.x, PlayerSize.y);
		}

		/* Scale. */
		ImGui::TableNextRow();
		{
			Label("Scale");
			NextColumn();
			ImGui::Text("(%.2f, %.2f)", TC.Scale.x, TC.Scale.y);
		}

		/* Linear velocity. */
		ImGui::TableNextRow();
		{
			const glm::vec2 LinearVelocity = Body->GetLinearVelocity();
			Label("Linear Velocity");
			NextColumn();
			ImGui::Text("(%.2f, %.2f)", LinearVelocity.x, LinearVelocity.y);
		}

		/* Angular velocity. */
		ImGui::TableNextRow();
		{
			const float AngularVelocity = Body->GetAngularVelocity();
			Label("Angular Velocity");
			NextColumn();
			ImGui::Text("%.2f", AngularVelocity);
		}

		/* Jump impulse. */
		ImGui::TableNextRow();
		float PlayerJumpImpulse = Player->GetJumpImpulse();
		Changed |= UI::Widget::DragFloat("Jump Impulse", PlayerJumpImpulse, 0.010f, 0.0f, 20.0f, "%.3f");
		if (Changed) {
			Player->SetJumpImpulse(PlayerJumpImpulse);
		}

		/* Direction force. */
		ImGui::TableNextRow();
		float DirForce = Player->GetDirectionForce();
		Changed |= UI::Widget::DragFloat("Direction Force", DirForce, 0.010f, 0.0f, 10.0f, "%.3f");
		if (Changed) {
			Player->SetDirectionForce(DirForce);
		}

		/* Last direction force. */
		ImGui::TableNextRow();
		{
			Label("Last Direction Force");
			NextColumn();
			ImGui::Text("%.3f", Player->GetLastDirectionForce());
		}

		/* Body scale. */
		ImGui::TableNextRow();
		{
			glm::vec2 BodySize = Body->GetSize();
			if (UI::Widget::DragFloat2("Body Size", BodySize, 0.10f, 0.010f, 0.10f, 2.0f)) {
				FPolygon& Shape = Body->GetShape<EShape::Polygon>();
				Shape.Size = BodySize;
			}

			ImGui::SameLine();
			if (ImGui::Button("Rebuild##Body")) {
				Body->Rebuild();
			}
			if (ImGui::IsItemHovered()) {
				ImGui::SameLine();
				ImGui::BeginTooltip();
				ImGui::TextColored(FColor::Convert<ImVec4>(RGBA32::Red), "!! NOT WORKING !!");
				ImGui::EndTooltip();
			}
		}

		/* Mass. */
		ImGui::TableNextRow();
		{
			float Mass = Body->GetMass();
			Changed |= UI::Widget::DragFloat("Mass", Mass, 0.010f, 0.0f, 10.0f, "%.2f");
			if (Changed) {
				Body->SetMass(Mass);
			}
		}

		/* Friction. */
		ImGui::TableNextRow();
		{
			float PlayerFriction = Body->GetFriction();
			Changed |= UI::Widget::DragFloat("Friction", PlayerFriction, 0.0f, 0.010f, 2.0f, "%.3f");
			if (Changed) {
				Body->SetFriction(PlayerFriction);
			}
		}

		/* Restitution. */
		ImGui::TableNextRow();
		{
			float Restitution = Body->GetRestitution();
			Changed |= UI::Widget::DragFloat("Restitution", Restitution, 0.010f, 0.0f, 2.0f, "%.3f");
			if (Changed) {
				Body->SetRestitution(Restitution);
			}
		}

		ImGui::EndTable();
		ImGui::Dummy(ImVec2(0, 8));

		if (ImGui::TreeNodeEx("Attributes", ImGuiTreeNodeFlags_SpanAvailWidth)) {
			UI::Widget::ActorNode::Data(Player);
			ImGui::TreePop();
		}
	}

	void RifleData(std::shared_ptr<CRifle> Rifle)
	{
		LK_ASSERT(Rifle);
		static constexpr float LabelColumnWidth = 180.0f;

		ImGui::BeginTable("##RifleData", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip);
		ImGui::TableSetupColumn("Label", 0, LabelColumnWidth);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, ImGui::GetContentRegionAvail().x - LabelColumnWidth);

		ImGui::TableNextRow();
		bool ShootEnabled = Rifle->IsEnabled();
		if (UI::Checkbox("Enabled", ShootEnabled)) {
			Rifle->SetEnabled(ShootEnabled);
		}

		ImGui::TableNextRow();
		float ProjectileRadius = Rifle->GetProjectileRadius();
		if (UI::Widget::DragFloat("Projectile Radius", ProjectileRadius, 0.0010f, 0.0010f, 1.0f)) {
			Rifle->SetProjectileRadius(ProjectileRadius);
		}

		ImGui::TableNextRow();
		float ProjectileVelocity = Rifle->GetProjectileVelocity();
		if (UI::Widget::DragFloat("Projectile Velocity", ProjectileVelocity, 0.10f, 0.0f, 20.0f)) {
			Rifle->SetProjectileVelocity(ProjectileVelocity);
		}

		ImGui::TableNextRow();
		bool ExplodeOnImpact = Rifle->GetProjectileExplodeOnImpact();
		if (UI::Checkbox("Explode On Impact", ExplodeOnImpact)) {
			Rifle->SetProjectileExplodeOnImpact(ExplodeOnImpact);
		}

		ImGui::TableNextRow();
		EColor ProjectileColor = EColor::Red;
		const bool ColorDeduced = FColor::DeduceEnum(ProjectileColor, Rifle->GetProjectileColor());
		if (!ColorDeduced) {
			ImGui::BeginDisabled();
		}
		if (UI::ColorDropdown(ProjectileColor)) {
			Rifle->SetProjectileColor(FColor::Get(ProjectileColor));
		}
		if (!ColorDeduced) {
			ImGui::EndDisabled();
		}

		ImGui::EndTable();
	}

	void Statistics(const EWidgetPlacement Placement)
	{
		CGameInstance* GameInstance = CGameInstance::Get();
		if (!GameInstance) {
			return;
		}

		const ImGuiStyle& Style = ImGui::GetStyle();

		float TopBarOffsetY = 0.0f;
		if (ImGuiWindow* Topbar = ImGui::FindWindowByName(PanelID::Topbar)) {
			TopBarOffsetY = Topbar->Size.y;
		}

		ImGui::SetNextWindowBgAlpha(0.25f);
		const ImVec2 WindowPos = ImGui::GetWindowPos();
		const float Padding = Style.FramePadding.x + Style.DockingSeparatorSize + Style.ItemSpacing.y;
		ImGui::SetNextWindowPos({WindowPos.x + Padding, Padding + TopBarOffsetY}, ImGuiCond_Always);

		ImGui::SetNextWindowSize(ImVec2(260, 0), ImGuiCond_Always);
		static constexpr ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoDecoration
			| ImGuiWindowFlags_NoDocking
			| ImGuiWindowFlags_NoBringToFrontOnFocus;
		if (!ImGui::Begin("##Statistics", nullptr, WindowFlags)) {
			ImGui::End();
			return;
		}

		std::shared_ptr<CPlayer> Player = GameInstance->GetPlayer();
		CCamera& Camera = Player->GetCamera();

		static constexpr float LabelColumnWidth = 150.0f;
		ImGui::BeginTable("##StatisticsTable", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip);
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

		/* FPS */
		const float DeltaTime = GameInstance->GetDeltaTime();
		const float FPS = 1.0f / DeltaTime;
		ImGui::TableNextRow();
		Label("FPS");
		NextColumn();
		ImGui::Text("%1.f", FPS);

		/* Camera zoom */
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

		if (ImGui::IsWindowHovered()) {
			const ImVec2 Pos = ImGui::GetWindowPos();
			const ImVec2 Size = ImGui::GetWindowSize();
			if (ImGui::BeginTooltip()) {
				ImGui::Text("DeltaTime: %.6f", DeltaTime);
				ImGui::Text("Position: (%.1f, %.1f)", Pos.x, Pos.y);
				ImGui::Text("Size: (%.1f, %.1f)", Size.x, Size.y);
				ImGui::EndTooltip();
			}
		}

		ImGui::End();
	}

	void PlayerHud(std::shared_ptr<CPlayer> Player)
	{
		if (!Player) {
			return;
		}

		const ImGuiStyle& Style = ImGui::GetStyle();
		ImGuiViewport* Viewport = ImGui::GetMainViewport();
		static constexpr ImVec2 WindowSize = ImVec2(380, 200);
		static constexpr ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs;

		const ImVec2 WindowPos = ImGui::GetWindowPos();
		const float PaddingX = Style.FramePadding.x + Style.DockingSeparatorSize + Style.ItemSpacing.y;
		float PaddingY = Style.FramePadding.x + Style.DockingSeparatorSize + Style.ItemSpacing.y;

		if (ImGuiWindow* BottomBar = ImGui::FindWindowByName(PanelID::BottomBar)) {
			PaddingY += BottomBar->Size.y;
		}

		const ImVec2 Avail = ImGui::GetContentRegionAvail();
		ImGui::SetNextWindowPos(ImVec2(WindowPos.x + PaddingX, Viewport->Size.y - (WindowSize.y + PaddingY)), ImGuiCond_Always);
		ImGui::SetNextWindowSize(WindowSize, ImGuiCond_Always);
		ImGui::SetNextWindowBgAlpha(0.40f);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
		if (!ImGui::Begin("##PlayerHud", nullptr, WindowFlags)) {
			ImGui::PopStyleVar(1);
			ImGui::End();
			return;
		}
		ImGui::PopStyleVar(1);

		static constexpr float LabelColumnWidth = 210.0f;
		ImGui::BeginTable("##PlayerHudTable", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip);
		ImGui::TableSetupColumn("Label", 0, LabelColumnWidth);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, ImGui::GetContentRegionAvail().x - LabelColumnWidth);

		auto Label = [](std::string_view Str) -> void
		{
			UI::FScopedFont Font(EFont::SourceSansPro, EFontSize::Header, EFontModifier::Bold);
			ImGui::TableSetColumnIndex(0);
			UI::ShiftCursor(17.0f, 4.0f);
			ImGui::AlignTextToFramePadding();
			ImGui::Text(Str.data());
		};

		auto NextColumn = []() -> void
		{
			ImGui::TableSetColumnIndex(1);
			ImGui::AlignTextToFramePadding();
			UI::ShiftCursor(0.0f, 4.0f);
		};

		/* Health */
		{
			static constexpr uint16_t HP = 100; /* @todo */
			ImGui::TableNextRow();
			Label("Health");

			NextColumn();
			UI::FScopedFont Font(EFont::SourceSansPro, EFontSize::Header, EFontModifier::Bold);
			std::uint32_t Color;
			if (HP > 50) {
				Color = RGBA32::LightGreen;
			} else if (HP > 25) {
				Color = RGBA32::Yellow;
			} else {
				Color = RGBA32::Red;
			}
			UI::FScopedColor TextColor(ImGuiCol_Text, Color);
			ImGui::Text("%d", HP);
		}

		/* Weapon */
		{
			std::shared_ptr<CRifle> Rifle = Player->GetRifle();
			ImGui::TableNextRow();
			Label("Weapon");

			NextColumn();
			UI::FScopedFont Font(EFont::SourceSansPro, EFontSize::Header, EFontModifier::Bold);
			if (Rifle) {
				const bool Enabled = Rifle->IsEnabled();
				if (!Enabled) {
					ImGui::PushStyleColor(ImGuiCol_Text, RGBA32::Gray);
				}
				ImGui::Text("Rifle");
				if (!Enabled) {
					ImGui::PopStyleColor(1);
				}
			} else {
				ImGui::Text("None");
			}

			if (Rifle) {
				ImGui::TableNextRow();
				Label("Ammo");

				NextColumn();
				const uint16_t Ammo = Rifle->GetAmmo();
				uint32_t Color = RGBA32::White;
				if (Ammo <= 3) {
					Color = RGBA32::Red;
				}
				UI::FScopedColor TextColor(ImGuiCol_Text, Color);
				ImGui::Text("%d", Ammo);
			}
		}

		/* Inventory info */
		{
			ImGui::TableNextRow();
			Label("Inventory");
			NextColumn();
			const CInventory& Inventory = Player->GetInventory();
			const std::size_t UsedSlots = Inventory.GetUsedSlots();
			UI::FScopedFont Font(EFont::SourceSansPro, EFontSize::Header, EFontModifier::Bold);
			ImGui::Text("%d/%d", UsedSlots, CInventory::MAX_ITEMS);
		}

		ImGui::EndTable();
		ImGui::End();
	}

	void EnemiesInfo(std::shared_ptr<CScene> Scene)
	{
		if (!Scene) {
			return;
		}

		const std::vector<std::shared_ptr<CEnemy>> Enemies = Scene->GetAllOfType<CEnemy>();
		if (Enemies.empty()) {
			return;
		}
		UI::HeaderTextCentralized("Enemies");
		for (auto& Enemy : Enemies) {
			UI::LargeText(Enemy->GetName());
			ImGui::Dummy(ImVec2(0, 4));
			UI::Widget::DrawEnemy(Enemy);
			ImGui::Dummy(ImVec2(0, 10));
		}
	}

	void PrepareLeftSidebar()
	{
		ImGuiWindow* SidebarWindow = ImGui::FindWindowByName(PanelID::Sidebar1);
		if (SidebarWindow == nullptr) {
			return;
		}

		ImGuiDockNode* DockNode = SidebarWindow->DockNode;
		if (DockNode == nullptr) {
			return;
		}
		if ((DockNode->Size.x <= 0.0f) || (DockNode->Size.y <= 0.0f)) {
			return;
		}

		auto& V = ViewportData;
		ImGuiViewport* Viewport = ImGui::GetWindowViewport();

		V.LeftSidebarSize = {DockNode->Size.x, DockNode->Size.y};
		SidebarWindow->Pos = ImVec2(0, V.MenuBarSize.y);
		SidebarWindow->Size = ImVec2(V.LeftSidebarSize.x, Viewport->WorkSize.y);

		DockNode->LocalFlags &= ~ImGuiDockNodeFlags_NoDockingSplit;

		/* Dock node has no other windows docked in it. */
		if (DockNode->Windows.Size <= 1) {
			DockNode->LocalFlags |= ImGuiDockNodeFlags_NoWindowMenuButton;
			DockNode->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
			SidebarWindow->Flags &= ~ImGuiWindowFlags_NoTitleBar;
			if (DockNode->VisibleWindow) {
				DockNode->VisibleWindow->Flags &= ~ImGuiWindowFlags_NoTitleBar;
			}
		} else if (DockNode->Windows.Size > 1) {
			DockNode->LocalFlags &= ~ImGuiDockNodeFlags_NoTabBar;
			DockNode->LocalFlags &= ~ImGuiDockNodeFlags_NoWindowMenuButton;

			if (DockNode->VisibleWindow) {
				DockNode->VisibleWindow->Flags &= ~ImGuiWindowFlags_NoTitleBar;
			}
		}
	}

	void PrepareRightSidebar()
	{
		ImGuiWindow* SidebarWindow = ImGui::FindWindowByName(PanelID::Sidebar2);
		if (SidebarWindow == nullptr) {
			return;
		}

		ImGuiDockNode* DockNode = SidebarWindow->DockNode;
		if (DockNode == nullptr) {
			return;
		}
		if ((DockNode->Size.x <= 0.0f) || (DockNode->Size.y <= 0.0f)) {
			return;
		}

		auto& V = ViewportData;
		ImGuiViewport* Viewport = ImGui::GetWindowViewport();

		V.RightSidebarSize = {DockNode->Size.x, DockNode->Size.y};
		SidebarWindow->Pos = ImVec2(Viewport->Size.x - V.RightSidebarSize.x, V.MenuBarSize.y);
		SidebarWindow->Size = ImVec2(V.RightSidebarSize.x, V.RightSidebarSize.y);

		DockNode->LocalFlags &= ~ImGuiDockNodeFlags_NoDocking;
		DockNode->LocalFlags &= ~ImGuiDockNodeFlags_NoDockingSplit;

		/* Dock node has no other windows docked in it. */
		if (DockNode->Windows.Size <= 1) {
			DockNode->LocalFlags |= ImGuiDockNodeFlags_NoWindowMenuButton;
			DockNode->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
			SidebarWindow->Flags &= ~ImGuiWindowFlags_NoTitleBar;

			if (DockNode->VisibleWindow) {
				DockNode->VisibleWindow->Flags &= ~ImGuiWindowFlags_NoTitleBar;
			}
		} else if (DockNode->Windows.Size > 1) {
			DockNode->LocalFlags &= ~ImGuiDockNodeFlags_NoTabBar;
			SidebarWindow->Flags &= ~ImGuiWindowFlags_NoTitleBar;

			if (DockNode->VisibleWindow) {
				DockNode->VisibleWindow->Flags &= ~ImGuiWindowFlags_NoTitleBar;
			}
		}
	}

	void PrepareTopBar()
	{
		ImGuiWindow* TopBarWindow = ImGui::FindWindowByName(PanelID::Topbar);
		if (TopBarWindow == nullptr) {
			return;
		}

		TopBarWindow->Flags |= ImGuiWindowFlags_NoTitleBar;
		ImGuiDockNode* DockNode = TopBarWindow->DockNode;
		if (DockNode == nullptr) {
			return;
		}
		if ((DockNode->Size.x <= 0.0f) || (DockNode->Size.y <= 0.0f)) {
			return;
		}

		DockNode->Size.y = 68.0f;
		DockNode->SizeRef.y = DockNode->Size.y; /* @fixme: Is this needed? */

		DockNode->LocalFlags |= ImGuiDockNodeFlags_NoDocking
			| ImGuiDockNodeFlags_NoWindowMenuButton
			| ImGuiDockNodeFlags_NoTabBar
			| ImGuiDockNodeFlags_NoResize;
	}

	void PrepareMenuBar()
	{
		ImGuiWindow* Window = ImGui::FindWindowByName(PanelID::Menubar);
		if (Window == nullptr) {
			return;
		}

		Window->Flags |= ImGuiWindowFlags_NoTitleBar;
		ImGuiDockNode* DockNode = Window->DockNode;
		if (DockNode == nullptr) {
			return;
		}
		if ((DockNode->Size.x <= 0.0f) || (DockNode->Size.y <= 0.0f)) {
			return;
		}

		DockNode->SizeRef.y = 28.0f;
		DockNode->LocalFlags |= ImGuiDockNodeFlags_NoDocking
			| ImGuiDockNodeFlags_NoWindowMenuButton
			| ImGuiDockNodeFlags_NoTabBar
			| ImGuiDockNodeFlags_NoResize;
	}

	void PrepareBottomBar()
	{
		ImGuiWindow* Window = ImGui::FindWindowByName(PanelID::BottomBar);
		if (Window == nullptr) {
			return;
		}

		ImGuiDockNode* DockNode = Window->DockNode;
		if (DockNode == nullptr) {
			return;
		}
		if ((DockNode->Size.x <= 0.0f) || (DockNode->Size.y <= 0.0f)) {
			return;
		}

		auto& V = ViewportData;
		ImGuiViewport* Viewport = ImGui::GetWindowViewport();

		V.RightSidebarSize = {DockNode->Size.x, DockNode->Size.y};
		Window->Pos = ImVec2(Viewport->Size.x - V.RightSidebarSize.x, V.MenuBarSize.y);
		Window->Size = ImVec2(V.RightSidebarSize.x, V.RightSidebarSize.y);

		DockNode->LocalFlags &= ~ImGuiDockNodeFlags_NoDocking;
		DockNode->LocalFlags &= ~ImGuiDockNodeFlags_NoDockingSplit;

		/* Dock node has no other windows docked in it. */
		if (DockNode->Windows.Size <= 1) {
			DockNode->LocalFlags |= ImGuiDockNodeFlags_NoWindowMenuButton;
			DockNode->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
			Window->Flags &= ~ImGuiWindowFlags_NoTitleBar;

			if (DockNode->VisibleWindow) {
				DockNode->VisibleWindow->Flags &= ~ImGuiWindowFlags_NoTitleBar;
			}
		} else if (DockNode->Windows.Size > 1) {
			DockNode->LocalFlags &= ~ImGuiDockNodeFlags_NoTabBar;
			Window->Flags &= ~ImGuiWindowFlags_NoTitleBar;

			if (DockNode->VisibleWindow) {
				DockNode->VisibleWindow->Flags &= ~ImGuiWindowFlags_NoTitleBar;
			}
		}
	}

}
