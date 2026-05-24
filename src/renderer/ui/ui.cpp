#include "ui.h"

#include <nfd.hpp>

#include "core/settings.h"
#include "core/window.h"
#include "core/input/keyboard.h"
#include "core/selectioncontext.h"
#include "game/instance.h"
#include "renderer/color.h"
#include "renderer/debugrenderer.h"
#include "renderer/font.h"
#include "renderer/renderer.h"
#include "combo.h"
#include "ui_core.h"
#include "widgets.h"
#include "game/rifle.h"
#include "scene/scene.h"

namespace platformer2d::UI {
	namespace {
		constexpr auto& ColorArray = FColor::GetArray(); /* @fixme: Remove */
	}

	void Table::Label(std::string_view Str, const float IndentX)
	{
		ImGui::TableSetColumnIndex(0); /* @todo: Remove */
		UI::ShiftCursorX(17.0f + IndentX);
		ImGui::AlignTextToFramePadding();
		ImGui::Text(Str.data());
	}

	void Table::Label(std::string_view Str, const EFont Font, const EFontSize FontSize, EFontModifier FontMod, const float IndentX)
	{
		UI::FScopedFont ScopedFont(Font, FontSize, FontMod);
		Label(Str, IndentX);
	}

	void Table::NextColumn()
	{
		ImGui::TableSetColumnIndex(1);
		UI::ShiftCursorX(7);
		ImGui::AlignTextToFramePadding();
	};

	void Table::NextRow()
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
	};

	bool Checkbox(std::string_view Str, bool& Value, const float IndentX)
	{
		bool Active = false;
		std::array<char, 64> LabelBuf = {0};
		std::snprintf(LabelBuf.data(), LabelBuf.size(), "##%s", Str.data());

		if (InTable()) {
			Table::Label(Str);
			Table::NextColumn();
			if (ImGui::Checkbox(LabelBuf.data(), &Value)) {
				Active = true;
			}
		} else {
			ImGui::Text(Str.data());
			ImGui::SameLine(0.0f, IndentX);
			if (ImGui::Checkbox(LabelBuf.data(), &Value)) {
				Active = true;
			}
		}

		return Active;
	}

	void DisabledCheckbox(std::string_view Str, bool& Value, const float IndentX)
	{
		bool Active = false;
		std::array<char, 64> LabelBuf = {0};
		std::snprintf(LabelBuf.data(), LabelBuf.size(), "##%s", Str.data());

		if (InTable()) {
			Table::Label(Str);
			Table::NextColumn();
			ImGui::BeginDisabled();
			ImGui::Checkbox(LabelBuf.data(), &Value);
			ImGui::EndDisabled();
		} else {
			ImGui::Text(Str.data());
			ImGui::SameLine(0.0f, IndentX);
			ImGui::BeginDisabled();
			ImGui::Checkbox(LabelBuf.data(), &Value);
			ImGui::EndDisabled();
		}
	}

	bool BeginPropertyGrid(const float LabelColumnWidth, const ImGuiTableColumnFlags ColumnFlags)
	{
		UI::PushID();
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 0));
		if (ImGui::BeginTable("##PropertyGrid", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip)) {
			ImGui::TableSetupColumn("L", 0, LabelColumnWidth);
			ImGui::TableSetupColumn("V", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip | ColumnFlags, ImGui::GetContentRegionAvail().x - LabelColumnWidth);
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
		/* @todo: Use UI::Table */
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

		return UI::Combo("##Color", Enum::View<EColor>(), Selected);
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
		const std::array<float, 3> SnapValues = {SnapValue, SnapValue, SnapValue};

		FTransformComponent& TC = Actor.GetTransformComponent();
		glm::mat4 TransformMatrix = TC.GetTransform();

		const bool Manipulated = ImGuizmo::Manipulate(
			glm::value_ptr(ViewMatrix),
			glm::value_ptr(ProjectionMatrix),
			static_cast<ImGuizmo::OPERATION>(Operation),
			ImGuizmo::WORLD,
			glm::value_ptr(TransformMatrix),
			nullptr,
			ShouldNotSnapValues ? nullptr : SnapValues.data());

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

	bool DrawTranslateGizmo(glm::vec2& Position, const glm::mat4& ViewMatrix, const glm::mat4& ProjectionMatrix)
	{
		ImGuizmo::SetOrthographic(true);
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowWidth(), ImGui::GetWindowHeight());

		const bool ShouldNotSnapValues = CKeyboard::IsKeyDown(EKey::LeftControl);
		constexpr float SnapValue = 0.010f;
		const std::array<float, 3> SnapValues = {SnapValue, SnapValue, SnapValue};

		glm::mat4 TransformMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(Position.x, Position.y, 0.0f));

		const bool Manipulated = ImGuizmo::Manipulate(
			glm::value_ptr(ViewMatrix),
			glm::value_ptr(ProjectionMatrix),
			ImGuizmo::OPERATION::TRANSLATE,
			ImGuizmo::WORLD,
			glm::value_ptr(TransformMatrix),
			nullptr,
			ShouldNotSnapValues ? nullptr : SnapValues.data());

		if (ImGuizmo::IsUsing()) {
			glm::vec3 Translation;
			glm::vec3 Scale;
			glm::quat Rotation;
			Math::DecomposeTransform(TransformMatrix, Translation, Rotation, Scale);
			Position.x = Translation.x;
			Position.y = Translation.y;
		}

		return Manipulated;
	}

	void DrawDivider(const float Width, const std::uint32_t Color)
	{
		const ImVec2 Cursor = ImGui::GetCursorScreenPos();
		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		DrawList->AddLine(ImVec2(Cursor.x, Cursor.y + 2.0f), ImVec2(Cursor.x + Width, Cursor.y + 2.0f), Color, 2.0f);
	}

	void PlayerData(std::shared_ptr<CPlayer>& Player)
	{
		if (!Player) {
			return;
		}
		CCamera& Camera = Player->GetCamera();
		CBody* Body = Player->GetBody();
		const FPlayerData& Data = Player->GetData();
		FTransformComponent& TC = Player->GetTransformComponent();

		BeginPropertyGrid();
		bool Changed = false;

		glm::vec2 BodyPos = Body->GetPosition();
		Table::NextRow();
		Table::Label("Position");
		Table::NextColumn();
		ImGui::Text("(%.2f, %.2f)", BodyPos.x, BodyPos.y);

		/* Movement state. */
		Table::NextRow();
		Table::Label("Movement State");
		Table::NextColumn();
		ImGui::Text("%s", Enum::ToString<const char*>(Data.MovementState));

		/* Jump state. */
		Table::NextRow();
		Table::Label("Jump State");
		Table::NextColumn();
		ImGui::Text("%s", Data.bJumping ? "Jumping" : "On ground");

		auto [CurrentSpriteFrame, NextSpriteFrame] = Player->GetCurrentAndNextSpriteFrame();
		Table::NextRow();
		Table::Label("Current Sprite Frame");
		Table::NextColumn();
		ImGui::Text("(%u, %u)", CurrentSpriteFrame.X, CurrentSpriteFrame.Y);

		Table::NextRow();
		Table::Label("Next Sprite Frame");
		Table::NextColumn();
		ImGui::Text("(%u, %u)", NextSpriteFrame.X, NextSpriteFrame.Y);

		const glm::vec2 PlayerSize = Player->GetSize();
		Table::NextRow();
		Table::Label("Size");
		Table::NextColumn();
		ImGui::Text("(%.2f, %.2f)", PlayerSize.x, PlayerSize.y);

		Table::NextRow();
		Table::Label("Scale");
		Table::NextColumn();
		ImGui::Text("(%.2f, %.2f)", TC.Scale.x, TC.Scale.y);

		Table::NextRow();
		const glm::vec2 LinearVelocity = Body->GetLinearVelocity();
		Table::Label("Linear Velocity");
		Table::NextColumn();
		ImGui::Text("(%.2f, %.2f)", LinearVelocity.x, LinearVelocity.y);

		Table::NextRow();
		const float AngularVelocity = Body->GetAngularVelocity();
		Table::Label("Angular Velocity");
		Table::NextColumn();
		ImGui::Text("%.2f", AngularVelocity);

		Table::NextRow();
		float PlayerJumpImpulse = Player->GetJumpImpulse();
		Changed |= UI::DragFloat("Jump Impulse", PlayerJumpImpulse, 0.010f, 0.0f, 20.0f, "%.3f");
		if (Changed) {
			Player->SetJumpImpulse(PlayerJumpImpulse);
		}

		Table::NextRow();
		float DirForce = Player->GetDirectionForce();
		Changed |= UI::DragFloat("Direction Force", DirForce, 0.010f, 0.0f, 10.0f, "%.3f");
		if (Changed) {
			Player->SetDirectionForce(DirForce);
		}

		Table::NextRow();
		Table::Label("Last Direction Force");
		Table::NextColumn();
		ImGui::Text("%.3f", Player->GetLastDirectionForce());

		Table::NextRow();
		glm::vec2 BodySize = Body->GetSize();
		if (UI::DragFloat2("Body Size", BodySize, 0.10f, 0.010f, 0.10f, 2.0f)) {
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

		Table::NextRow();
		float Mass = Body->GetMass();
		Changed |= UI::DragFloat("Mass", Mass, 0.010f, 0.0f, 10.0f, "%.2f");
		if (Changed) {
			Body->SetMass(Mass);
		}

		Table::NextRow();
		float PlayerFriction = Body->GetFriction();
		Changed |= UI::DragFloat("Friction", PlayerFriction, 0.0f, 0.010f, 2.0f, "%.3f");
		if (Changed) {
			Body->SetFriction(PlayerFriction);
		}

		/* Restitution. */
		ImGui::TableNextRow();
		{
			float Restitution = Body->GetRestitution();
			Changed |= UI::DragFloat("Restitution", Restitution, 0.010f, 0.0f, 2.0f, "%.3f");
			if (Changed) {
				Body->SetRestitution(Restitution);
			}
		}

		EndPropertyGrid();

		ImGui::Spacing();
		if (ImGui::TreeNodeEx("Attributes", ImGuiTreeNodeFlags_SpanAvailWidth)) {
			UI::Actor::Data(Player);
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
		if (UI::DragFloat("Projectile Radius", ProjectileRadius, 0.0010f, 0.0010f, 1.0f)) {
			Rifle->SetProjectileRadius(ProjectileRadius);
		}

		ImGui::TableNextRow();
		float ProjectileVelocity = Rifle->GetProjectileVelocity();
		if (UI::DragFloat("Projectile Velocity", ProjectileVelocity, 0.10f, 0.0f, 20.0f)) {
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
		if (!CGameInstance::IsValid()) {
			return;
		}

		auto& GameInstance = CGameInstance::Get();
		const ImGuiStyle& Style = ImGui::GetStyle();

		float TopBarOffsetY = 0.0f;
		if (ImGuiWindow* Topbar = ImGui::FindWindowByName(PanelID::Topbar)) {
			TopBarOffsetY = Topbar->Size.y + ImGui::GetFrameHeight();
		}

		ImVec2 ContentMin;
		if (ImGuiWindow* EditorViewport = ImGui::FindWindowByName(PanelID::Viewport)) {
			ContentMin = EditorViewport->InnerRect.Min;
		} else {
			const ImVec2 WindowPos = ImGui::GetWindowPos();
			ContentMin = ImVec2(WindowPos.x, WindowPos.y + TopBarOffsetY);
		}

		constexpr float LabelColumnWidth = 120.0f;

		ImGui::SetNextWindowBgAlpha(0.25f);
		const float Padding = Style.FramePadding.x + Style.DockingSeparatorSize + Style.ItemSpacing.y;
		ImGui::SetNextWindowPos(ImVec2(ContentMin.x + Padding, ContentMin.y + Padding), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(LabelColumnWidth * 2.0f, 0), ImGuiCond_Always);
		constexpr ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoSavedSettings
			| ImGuiWindowFlags_NoDecoration
			| ImGuiWindowFlags_NoDocking;
		if (!UI::Begin("##Statistics", nullptr, WindowFlags)) {
			return;
		}

		std::shared_ptr<CPlayer> Player = GameInstance.GetPlayer();
		CCamera& Camera = Player->GetCamera();

		const auto& GraphicsSettings = FSettings::Get().Graphics;
		const float DeltaTime = GameInstance.GetDeltaTime();
		const float FPS = (DeltaTime > 0.0f) ? (1.0f / DeltaTime) : 0.0f;
		struct
		{
			float Value = 0.0f;
			bool Cached = false;
		} static CachedFPS = {.Value = FPS};

		UI::BeginPropertyGrid(LabelColumnWidth);

		if (GraphicsSettings.bShowFPS) {
			Table::NextRow();
			Table::Label("FPS");
			Table::NextColumn();
			if (!UI::IsPauseMenuOpen()) {
				ImGui::Text("%1.f", FPS);
			} else {
				ImGui::Text("%1.f", CachedFPS.Value);
			}
		}

		if (GraphicsSettings.bShowFrametime) {
			Table::NextRow();
			Table::Label("Frametime");
			Table::NextColumn();
			ImGui::Text("%.2f ms", DeltaTime * 1000.0f);
		}

		if (GraphicsSettings.bShowDebugStats) {
			const FDrawStatistics& Stats = CRenderer::GetDrawStatistics();
			Table::NextRow();
			Table::Label("Quads");
			Table::NextColumn();
			ImGui::Text("%llu", Stats.QuadCount);

			Table::NextRow();
			Table::Label("Lines");
			Table::NextColumn();
			ImGui::Text("%llu", Stats.LineCount);
		}

		UI::EndPropertyGrid();

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

		/* @todo: Caching is broken */
		if (UI::IsPauseMenuOpen()) {
			if (!CachedFPS.Cached) {
				CachedFPS.Value = FPS;
				CachedFPS.Cached = true;
			}
		} else {
			CachedFPS.Cached = false;
		}

		UI::End();
	}

	void EnemiesInfo(std::shared_ptr<CScene>& Scene)
	{
		if (!Scene) {
			return;
		}
		const std::vector<std::shared_ptr<CEnemy>> Enemies = Scene->GetAllOfType<CEnemy>();
		if (Enemies.empty()) {
			return;
		}

		UI::HeaderTextCentralized("Enemies");
		const bool DefaultOpen = (Enemies.size() == 1);
		for (auto& Enemy : Enemies) {
			LK_ASSERT(Enemy);
			const LUUID Handle = Enemy->GetHandle();
			UI::FScopedID ScopedID(Handle);

			ImGui::SetNextItemOpen(DefaultOpen, ImGuiCond_Once);
			std::string_view Name = Enemy->GetName();
			const bool TreeOpen = ImGui::TreeNodeEx(
				Name.empty() ? "Enemy" : Name.data(),
				ImGuiTreeNodeFlags_SpanAvailWidth);
			if (TreeOpen) {
				ImGui::Dummy(ImVec2(0, 4));
				UI::DrawEnemy(Enemy);
				ImGui::TreePop();
			}
		}
	}

	void SpriteSheetModification(std::shared_ptr<CPlayer>& Player)
	{
		static std::vector<ETexture> SheetTextures;
		static std::vector<const char*> SheetLabels;
		static constexpr auto AllTextures = Enum::View<ETexture>();
		if (SheetTextures.empty()) {
			SheetTextures.reserve(AllTextures.size());
			SheetLabels.reserve(AllTextures.size());
			for (const ETexture T : AllTextures) {
				if (CRenderer::GetSpriteSheet(T) != nullptr) {
					SheetTextures.push_back(T);
					SheetLabels.push_back(Enum::ToString<const char*>(T));
				}
			}
		}

		if (!Player || SheetTextures.empty()) {
			return;
		}

		ImGui::Dummy(ImVec2(0, 6));
		ImGui::TextUnformatted("Sprite Sheet");
		ImGui::SameLine(0, 6);

		const ETexture CurrentTexture = Player->GetTexture();
		int CurrentIdx = 0;
		for (std::size_t Idx = 0; Idx < SheetTextures.size(); Idx++) {
			if (SheetTextures[Idx] == CurrentTexture) {
				CurrentIdx = static_cast<int>(Idx);
				break;
			}
		}
		ImGui::SetNextItemWidth(140.0f);
		if (ImGui::Combo("##PlayerSpriteSheet", &CurrentIdx, SheetLabels.data(), static_cast<int>(SheetLabels.size()))) {
			const ETexture Selected = SheetTextures[CurrentIdx];
			if (Selected != CurrentTexture) {
				Player->SetSpriteSheet(Selected);
			}
		}

		constexpr float ITEM_WIDTH = 130.0f;

		FSpriteSheet* MutableSheet = CRenderer::GetSpriteSheetMutable(Player->GetTexture());
		if (MutableSheet != nullptr) {
			ImGui::Dummy(ImVec2(0, 6));
			ImGui::TextUnformatted("Tile Size (px)");
			ImGui::SameLine(0, 6);
			std::array<int, 2> TileSize = {static_cast<int>(MutableSheet->TileSize.x), static_cast<int>(MutableSheet->TileSize.y)};
			ImGui::SetNextItemWidth(140.0f);
			if (ImGui::DragInt2("##PlayerSpriteTileSize", TileSize.data(), 1.0f, 1, 1024)) {
				MutableSheet->TileSize.x = static_cast<float>(std::max(1, TileSize[0]));
				MutableSheet->TileSize.y = static_cast<float>(std::max(1, TileSize[1]));
			}
			ImGui::SameLine(0, 6);
			if (ImGui::Button("Re-apply##PlayerSprite")) {
				Player->SetSpriteSheet(Player->GetTexture());
			}

			static int FrameIdx = 0;
			static std::vector<ESpriteFrame> FrameKeys;
			static std::vector<const char*> FrameLabels;
			FrameKeys.clear();
			FrameLabels.clear();
			FrameKeys.reserve(MutableSheet->Animations.size());
			FrameLabels.reserve(MutableSheet->Animations.size());
			for (const auto& [Key, Anim] : MutableSheet->Animations) {
				FrameKeys.push_back(Key);
				FrameLabels.push_back(Enum::ToString<const char*>(Key));
			}

			if (!FrameKeys.empty()) {
				if (FrameIdx >= static_cast<int>(FrameKeys.size())) {
					FrameIdx = 0;
				}
				ImGui::Dummy(ImVec2(0, 4));
				ImGui::TextUnformatted("Anim Frame");
				ImGui::SameLine(0, 6);
				ImGui::SetNextItemWidth(ITEM_WIDTH);
				ImGui::Combo("##PlayerAnimFrame", &FrameIdx, FrameLabels.data(), static_cast<int>(FrameLabels.size()));

				FSpriteAnimation& Anim = MutableSheet->Animations.at(FrameKeys[FrameIdx]);
				std::array<int, 2> StartTile = {Anim.StartTileX, Anim.StartTileY};
				int FrameCount = static_cast<int>(Anim.FrameCount);
				int TicksPerFrame = Anim.TicksPerFrame;

				ImGui::SetNextItemWidth(ITEM_WIDTH);
				if (ImGui::DragInt2("StartTile##PlayerAnim", StartTile.data(), 1.0f, 0, 4096)) {
					Anim.StartTileX = static_cast<std::uint16_t>(std::max(0, StartTile[0]));
					Anim.StartTileY = static_cast<std::uint16_t>(std::max(0, StartTile[1]));
				}
				ImGui::SetNextItemWidth(ITEM_WIDTH);
				if (ImGui::DragInt("FrameCount##PlayerAnim", &FrameCount, 1.0f, 1, 64)) {
					Anim.FrameCount = static_cast<std::size_t>(std::max(1, FrameCount));
				}
				ImGui::SetNextItemWidth(ITEM_WIDTH);
				if (ImGui::DragInt("TicksPerFrame##PlayerAnim", &TicksPerFrame, 1.0f, 1, 240)) {
					Anim.TicksPerFrame = static_cast<std::uint16_t>(std::max(1, TicksPerFrame));
				}
			}
		}

		ImGui::Dummy(ImVec2(0, 6));
		ImGui::TextUnformatted("Sprite Scale");
		ImGui::SameLine(0, 6);
		float Scale = Player->GetSpriteScale();
		ImGui::SetNextItemWidth(ITEM_WIDTH);
		if (ImGui::SliderFloat("##PlayerSpriteScale", &Scale, 0.25f, 6.0f, "%.2f")) {
			Player->SetSpriteScale(Scale);
		}

		ImGui::TextUnformatted("Sprite Offset");
		ImGui::SameLine(0, 6);
		glm::vec2 Offset = Player->GetSpriteOffset();
		std::array<float, 2> OffsetArr = {Offset.x, Offset.y};
		ImGui::SetNextItemWidth(ITEM_WIDTH);
		if (ImGui::DragFloat2("##PlayerSpriteOffset", OffsetArr.data(), 0.005f, -2.0f, 2.0f, "%.3f")) {
			Player->SetSpriteOffset({OffsetArr[0], OffsetArr[1]});
		}
	}

	bool Input::ActorName(CActor& Actor)
	{
		auto Iter = ActorCache.Find(Actor.GetHandle());
		if (Iter == ActorCache.End()) {
			return false;
		}

		if (UI::InTable()) {
			Table::NextRow();
			Table::Label("Name");
			Table::NextColumn();
		}

		Internal::FActorDataEntry& Data = Iter->second;
		ImGui::SetNextItemWidth(200.0f);
		ImGui::InputText("##ActorName", Data.NameBuf.data(), Data.NameBuf.size());
		ImGui::SameLine();

		UI::FScopedFont Font(UI::Font::Get(EFont::SourceSansPro, EFontSize::Regular, EFontModifier::Bold));
		UI::FScopedStyleStack StyleStack(
			ImGuiStyleVar_FramePadding, ImVec2(5, 5),
			ImGuiStyleVar_FrameRounding, 3.0f);
		if (ImGui::Button(LK_ICON_CHECK_CIRCLE)) {
			LK_DEBUG_TAG("UI", "Rename {} to: {}", Actor.GetHandle(), Data.NameBuf.data());
			Actor.SetName(Data.NameBuf.data());
			return true;
		}

		return false;
	}

	bool PickImageFile(std::filesystem::path& OutPath)
	{
		NFD::Guard NfdGuard;
		NFD::UniquePathU8 Picked;
		static constexpr nfdu8filteritem_t Filters[] = {
			{"Image", "png,jpg,jpeg,tga,bmp,hdr"},
		};
		const nfdresult_t Result = NFD::OpenDialog(Picked, Filters, 1, TEXTURES_DIR);
		if (Result != NFD_OKAY) {
			if (Result == NFD_ERROR) {
				const char* Err = NFD::GetError();
				LK_ERROR_TAG("AssetInspector", "NFD error: {}", (Err ? Err : "unknown"));
			}
			return false;
		}
		OutPath = std::filesystem::path(Picked.get());
		return true;
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

		ImGuiViewport* Viewport = ImGui::GetWindowViewport();
		const ImVec2 LeftSidebarSize = {DockNode->Size.x, DockNode->Size.y};
		SidebarWindow->Pos = ImVec2(0, 0);
		SidebarWindow->Size = ImVec2(LeftSidebarSize.x, Viewport->WorkSize.y);

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

		ImGuiViewport* Viewport = ImGui::GetWindowViewport();
		const ImVec2 RightSidebarSize = {DockNode->Size.x, DockNode->Size.y};
		SidebarWindow->Pos = ImVec2(Viewport->Size.x - RightSidebarSize.x, 0);
		SidebarWindow->Size = ImVec2(RightSidebarSize.x, RightSidebarSize.y);

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

		ImGuiViewport* Viewport = ImGui::GetWindowViewport();
		const ImVec2 BottomBar = {DockNode->Size.x, DockNode->Size.y};
		Window->Pos = ImVec2(Viewport->Size.x - BottomBar.x, 0);
		Window->Size = ImVec2(BottomBar.x, BottomBar.y);

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

