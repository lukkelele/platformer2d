#include "widgets.h"

#include "core/window.h"
#include "core/input/keyboard.h"
#include "game/instance.h"
#include "core/selectioncontext.h"
#include "game/spawner.h"
#include "game/controller/patrolcontroller.h"
#include "editor_resources.h"
#include "enemytools.h"
#include "imgui.h"
#include "renderer/fontawesome.h"
#include "ui.h"
#include "scene/scene.h"

namespace platformer2d::UI {

	namespace {
		struct FBodyEditState
		{
			FBodySpecification Spec;
			bool bSeeded = false;
			bool bDirty = false;
		};
	}

	static std::unordered_map<LUUID, FBodyEditState> BodyEditCache;

	bool FlagChip(const char* Label, const bool Active, const std::uint32_t Accent)
	{
		const ImColor Base = Active ? ImColor(Accent) : ImColor(RGBA32::BackgroundDark);
		UI::FScopedStyle Rounding(ImGuiStyleVar_FrameRounding, 9.0f);
		UI::FScopedStyle Padding(ImGuiStyleVar_FramePadding, ImVec2(9, 3));
		UI::FScopedColorStack Colors(
			ImGuiCol_Button, Base.Value,
			ImGuiCol_ButtonHovered, UI::ColorWithMultipliedValue(Base, 1.25f).Value,
			ImGuiCol_ButtonActive, UI::ColorWithMultipliedValue(Base, 0.85f).Value);
		UI::FScopedColor TextColor(ImGuiCol_Text, Active ? FColor::Black.As<std::uint32_t>() : RGBA32::Text::Brighter);
		return ImGui::Button(Label);
	}

	static const char* ActorTypeIcon(const EActorType Type)
	{
		switch (Type) {
			case EActorType::Player:     return LK_ICON_MALE;
			case EActorType::Enemy:      return LK_ICON_CROSSHAIRS;
			case EActorType::Spawnpoint: return LK_ICON_BULLSEYE;
			case EActorType::Projectile: return LK_ICON_DOT_CIRCLE_O;
			case EActorType::Object:
			default:                     return LK_ICON_CUBE;
		}
	}

	static bool ContainsCI(std::string_view Haystack, std::string_view Needle)
	{
		if (Needle.empty()) {
			return true;
		}
		if (Needle.size() > Haystack.size()) {
			return false;
		}

		const auto ToLower = [](const char C)
		{
			return static_cast<char>(std::tolower(static_cast<unsigned char>(C)));
		};
		const auto Iter = std::search(
			Haystack.begin(), Haystack.end(),
			Needle.begin(), Needle.end(),
			[&](const char A, const char B)
		{
			return ToLower(A) == ToLower(B);
		});
		return (Iter != Haystack.end());
	}

	void Actor::Buttons(std::shared_ptr<CActor> Actor, std::shared_ptr<CScene> Scene)
	{
		LK_ASSERT(Actor && Scene);
		if (!Actor || !Scene) {
			return;
		}

		/* Button: Create */
		{
			const ImVec2 Avail = ImGui::GetContentRegionAvail();
			constexpr ImVec2 ButtonSize = ImVec2(112, 50);

			ImGui::BeginTable("##ActorButtons", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip);
			ImGui::TableSetupColumn("L", 0, Avail.x * 0.30f);
			ImGui::TableSetupColumn("V", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, Avail.x * 0.60);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(1);

			UI::FScopedFont Font(UI::Font::Get(EFont::SourceSansPro, EFontSize::Large, EFontModifier::Bold));
			UI::FScopedStyle ButtonFrame(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
			UI::FScopedStyle ButtonRounding(ImGuiStyleVar_FrameRounding, 8);
			{
				UI::FScopedColorStack ButtonColor(
					ImGuiCol_Button, RGBA32::LightGreen,
					ImGuiCol_ButtonHovered, RGBA32::DarkGreen,
					ImGuiCol_ButtonActive, RGBA32::NiceGreen);

				const bool ActorExists = Scene->DoesActorExist(ActorAttr.NameBuf.data());
				if (ActorExists) {
					ImGui::BeginDisabled();
				}
				const float CursorPosX = 0.50f * ImGui::GetContentRegionAvail().x;
				UI::ShiftCursorX(CursorPosX - ButtonSize.x);
				if (ImGui::Button("Create", ButtonSize)) {
					FBodySpecification NewBodySpec;
					Aggregate(PhysicsBodyData, NewBodySpec);
					LK_INFO("{}", CBody::ToString(NewBodySpec));
					CSpawner::CreatePolygon(
						ActorAttr.NameBuf.data(),
						NewBodySpec,
						ActorAttr.Size,
						FColor::Get(ActorAttr.Color));
				}
				if (ActorExists) {
					ImGui::EndDisabled();
				}
			}

			/* Button: Delete */
			{
				ImGui::SameLine();
				UI::FScopedColorStack ButtonColours(
					ImGuiCol_Button, RGBA32::WineRed,
					ImGuiCol_ButtonHovered, RGBA32::DarkRed,
					ImGuiCol_ButtonActive, RGBA32::Red);

				UI::ShiftCursorX(ImGui::GetStyle().FramePadding.x);
				const bool IsDeletable = Actor->IsDeletable();
				if (!IsDeletable) {
					ImGui::BeginDisabled();
				}
				if (ImGui::Button("Delete", ButtonSize)) {
					const LUUID ActorHandle = Actor->GetHandle();
					LK_INFO("Delete: {} ({})", ActorHandle, Actor->GetName());
					Scene->DeleteActor(ActorHandle);
				}
				if (!IsDeletable) {
					ImGui::EndDisabled();
				}
			}

			ImGui::EndTable();
		}
	}

	void Actor::DeleteButton(std::shared_ptr<CActor> Actor, std::shared_ptr<CScene> Scene)
	{
		LK_ASSERT(Actor && Scene);
		if (!Actor || !Scene) {
			return;
		}

		const ImVec2 Avail = ImGui::GetContentRegionAvail();
		constexpr ImVec2 ButtonSize = ImVec2(138, 50);

		UI::FScopedFont Font(UI::Font::Get(EFont::SourceSansPro, EFontSize::Large, EFontModifier::Bold));
		UI::FScopedStyle ButtonFrame(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
		UI::FScopedStyle ButtonRounding(ImGuiStyleVar_FrameRounding, 8);
		UI::FScopedColorStack ButtonColours(
			ImGuiCol_Button, RGBA32::WineRed,
			ImGuiCol_ButtonHovered, RGBA32::DarkRed,
			ImGuiCol_ButtonActive, RGBA32::Red);

		UI::ShiftCursorX(ImGui::GetStyle().FramePadding.x);
		const bool IsDeletable = Actor->IsDeletable();
		if (!IsDeletable) {
			ImGui::BeginDisabled();
		}
		if (ImGui::Button("Delete Actor", ButtonSize)) {
			const LUUID ActorHandle = Actor->GetHandle();
			LK_INFO("Delete: {} ({})", ActorHandle, Actor->GetName());
			Scene->DeleteActor(ActorHandle);
		}
		if (!IsDeletable) {
			ImGui::EndDisabled();
		}
	}

	static void Node_BodyLiveInfo(CActor& Actor, CBody& Body)
	{
		if (!ImGui::TreeNodeEx("Live", ImGuiTreeNodeFlags_SpanAvailWidth)) {
			return;
		}

		BeginPropertyGrid();

		Table::NextRow();
		bool Enabled = Body.IsEnabled();
		if (UI::Checkbox("Enabled", Enabled)) {
			Body.SetEnabled(Enabled);
		}

		Table::NextRow();
		bool TickEnabled = Actor.IsTickEnabled();
		if (UI::Checkbox("Tick", TickEnabled)) {
			Actor.SetTickEnabled(TickEnabled);
		}

		Table::NextRow();
		Table::Label("Awake");
		Table::NextColumn();
		ImGui::Text("%s", Body.IsAwake() ? "Yes" : "No");

		Table::NextRow();
		Table::Label("Current Size");
		Table::NextColumn();
		const glm::vec2 BodySize = Body.GetSize();
		ImGui::Text("(%.2f, %.2f)", BodySize.x, BodySize.y);

		Table::NextRow();
		Table::Label("Linear Velocity");
		Table::NextColumn();
		const glm::vec2 LinearV = Body.GetLinearVelocity();
		ImGui::Text("(%.2f, %.2f)", LinearV.x, LinearV.y);

		Table::NextRow();
		Table::Label("Angular Velocity");
		Table::NextColumn();
		ImGui::Text("%.2f", Body.GetAngularVelocity());

		Table::NextRow();
		Table::Label("AABB");
		Table::NextColumn();
		const FAABB AABB = Body.GetAABB();
		ImGui::Text("Min (%.2f, %.2f)", AABB.Min.x, AABB.Min.y);
		ImGui::SameLine(0.0f, 16.0f);
		ImGui::Text("Max (%.2f, %.2f)", AABB.Max.x, AABB.Max.y);

		EndPropertyGrid();
		ImGui::TreePop();
	}

	static void Node_BodyEditor(CActor& Actor, CBody& Body, FBodyEditState& Edit)
	{
		BeginPropertyGrid();

		Table::NextRow();
		Table::Label("Type");
		Table::NextColumn();
		ImGui::SetNextItemWidth(-1.0f);
		EBodyType Type = Edit.Spec.Type;
		if (UI::Combo("##BodyType", Enum::View<EBodyType>(), Type)) {
			Edit.Spec.Type = Type;
			Edit.bDirty = true;
		}

		Table::NextRow();
		glm::vec2 Size = Body.GetSize();
		if (UI::DragFloat2("Size", Size, 1.0f, 0.01f, 0.010f, 50.0f)) {
			Size.x = std::max(Size.x, 0.010f);
			Size.y = std::max(Size.y, 0.010f);
			Actor.SetSize(Size);
			if (FPolygon* Polygon = std::get_if<FPolygon>(&Edit.Spec.Shape)) {
				Polygon->Size = Size;
			}
		}

		Table::NextRow();
		if (UI::DragFloat("Gravity Scale", Edit.Spec.GravityScale, 0.01f, 0.0f, 5.0f, "%.2f")) {
			Edit.bDirty = true;
		}

		Table::NextRow();
		if (UI::DragFloat("Friction", Edit.Spec.Friction, 0.01f, 0.0f, 2.0f, "%.2f")) {
			Edit.bDirty = true;
		}

		Table::NextRow();
		if (UI::DragFloat("Density", Edit.Spec.Density, 0.01f, 0.0f, 10.0f, "%.2f")) {
			Edit.bDirty = true;
		}

		Table::NextRow();
		if (UI::DragFloat("Linear Damping", Edit.Spec.LinearDamping, 0.01f, 0.0f, 10.0f, "%.2f")) {
			Edit.bDirty = true;
		}

		Table::NextRow();
		if (UI::DragFloat("Angular Damping", Edit.Spec.AngularDamping, 0.01f, 0.0f, 10.0f, "%.2f")) {
			Edit.bDirty = true;
		}

		Table::NextRow();
		bool Sensor = Edit.Spec.bSensor;
		if (UI::Checkbox("Sensor", Sensor)) {
			Edit.Spec.bSensor = Sensor;
			Edit.bDirty = true;
		}

		EndPropertyGrid();

		/* Body flags as toggleable chips. */
		ImGui::Dummy(ImVec2(0, 2));
		ImGui::AlignTextToFramePadding();
		UI::ShiftCursorX(4.0f);
		ImGui::TextUnformatted("Flags");
		ImGui::SameLine(0.0f, 10.0f);

		FChipRow Row;
		const auto BodyChip = [&](const char* Label, const EBodyFlag Flag)
		{
			Row.Next(Label);
			const auto Bit = static_cast<std::underlying_type_t<EBodyFlag>>(Flag);
			const bool On = (Edit.Spec.Flags & Bit) != 0;
			if (FlagChip(Label, On, RGBA32::Orange)) {
				if (On) {
					Edit.Spec.Flags &= ~Bit;
				} else {
					Edit.Spec.Flags |= Bit;
				}
				Edit.bDirty = true;
			}
		};
		BodyChip("PreSolve", EBodyFlag_PreSolveEvents);
		BodyChip("Contact", EBodyFlag_ContactEvents);
		BodyChip("Sensor", EBodyFlag_SensorEvents);
		BodyChip("Bullet", EBodyFlag_Bullet);

		/* Rebuild button. */
		ImGui::Dummy(ImVec2(0, 6));
		{
			const ImColor ButtonColor = Edit.bDirty ? ImColor(RGBA32::NiceGreen) : ImColor(RGBA32::Background);
			UI::FScopedStyle Rounding(ImGuiStyleVar_FrameRounding, 6.0f);
			UI::FScopedStyle Padding(ImGuiStyleVar_FramePadding, ImVec2(6, 6));
			UI::FScopedColorStack Colors(
				ImGuiCol_Button, ButtonColor.Value,
				ImGuiCol_ButtonHovered, UI::ColorWithMultipliedValue(ButtonColor, 1.20f).Value,
				ImGuiCol_ButtonActive, UI::ColorWithMultipliedValue(ButtonColor, 0.90f).Value);

			std::array<char, 64> Label = {0};
			std::snprintf(Label.data(), Label.size(), "%s  Rebuild Body", LK_ICON_REFRESH);
			if (ImGui::Button(Label.data(), ImVec2(-1.0f, 0.0f))) {
				Actor.ReplaceBody(Edit.Spec);
				Edit.Spec = Body.GetSpecification();
				Edit.bDirty = false;
			}
		}

		ImGui::Dummy(ImVec2(0, 4));
		Node_BodyLiveInfo(Actor, Body);
	}

	static bool Node_Outline(CActor& Actor)
	{
		if (!ImGui::TreeNodeEx("Outline", ImGuiTreeNodeFlags_SpanAvailWidth)) {
			return false;
		}

		BeginPropertyGrid();
		Table::NextRow();
		Table::Label("Enabled");
		Table::NextColumn();
		bool Enabled = Actor.IsOutlineEnabled();
		if (ImGui::Checkbox("##Enabled", &Enabled)) {
			Actor.SetOutlineEnabled(Enabled);
		}

		Table::NextRow();
		float Thickness = Actor.GetOutlineThickness();
		if (UI::DragFloat("Thickness", Thickness, 0.10f, 0.0f, 20.0f)) {
			Actor.SetOutlineThickness(Thickness);
		}

		Table::NextRow();
		const glm::vec4& Color = Actor.GetOutlineColor();
		glm::vec3 C = {Color.x, Color.y, Color.z};
		if (UI::DragFloat3("Outline Color", C, 1.0f, 0.010f, 0.0f, 1.0f)) {
			Actor.SetOutlineColor(glm::vec4(C, 1.0f));
		}

		UI::EndPropertyGrid();
		ImGui::TreePop();
		return true;
	}

	void Actor::Data(std::shared_ptr<CActor> Actor)
	{
		const LUUID Handle = Actor->GetHandle();
		FScopedID ScopedID(Handle);

		/* Cache the actor, done to be able to rename it. */
		if (!ActorCache.Contains(*Actor)) {
			ActorCache.Cache(*Actor);
		}

		CBody* Body = Actor->GetBody();

		BeginPropertyGrid();
		Input::ActorName(*Actor);

		Table::NextRow();
		ETexture Texture = Actor->GetTexture();
		if (UI::TextureDropdown(Texture)) {
			LK_INFO_TAG("UI", "Update {} texture: {}", Actor->GetName(), Enum::ToString(Texture));
			Actor->SetTexture(Texture);
		}

		Table::NextRow();
		const glm::vec4& ColorRef = Actor->GetColor();
		EColor Color = EColor::White;
		const bool ColorDeduced = FColor::DeduceEnum(Color, ColorRef);
		if (!ColorDeduced) {
			LK_WARN_TAG("UI", "Failed to deduce: {}", ColorRef);
			ImGui::BeginDisabled();
		}
		const bool ColorUpdated = ColorDropdown(Color, -78.0f);
		if (ColorUpdated) {
			LK_INFO_TAG("UI", "Update {} color: {}", Actor->GetName(), Enum::ToString(Color));
			Actor->SetColor(FColor::Get(Color));
		}
		ImGui::SameLine();
		glm::vec4 ColorValue = Actor->GetColor();
		ImGui::SetNextItemWidth(50.0f);
		if (ImGui::SliderFloat("A##ColorAlpha", &ColorValue.a, 0.0f, 1.0f, "%.2f") || ColorUpdated) {
			Actor->SetColor(ColorValue);
		}
		if (!ColorDeduced) {
			ImGui::EndDisabled();
		}

		EndPropertyGrid();

		ImGui::Dummy(ImVec2(0, 2));
		ImGui::AlignTextToFramePadding();
		UI::ShiftCursorX(4.0f);
		ImGui::TextUnformatted("Flags");
		ImGui::SameLine(0.0f, 10.0f);
		{
			FChipRow Row;
			const auto ActorChip = [&](const char* Label, const EActorFlag Flag)
			{
				Row.Next(Label);
				const bool On = Actor->HasFlag(Flag);
				if (FlagChip(Label, On, RGBA32::Highlight)) {
					Actor->SetFlag(Flag, !On);
				}
			};
			ActorChip("Transparent", EActorFlag_Transparent);
			ActorChip("Terrain", EActorFlag_Terrain);
			ActorChip("Spawnpoint", EActorFlag_Spawnpoint);
		}

		ImGui::Dummy(ImVec2(0, 4));
		DrawComponents(Actor);

		if (Body) {
			FBodyEditState& Edit = BodyEditCache[Handle];
			if (!Edit.bSeeded) {
				Edit.Spec = Body->GetSpecification();
				Edit.bSeeded = true;
			}

			ImGui::Dummy(ImVec2(0, 4));
			ImGui::PushFont(UI::Font::Get(EFont::SourceSansPro, EFontSize::Large, EFontModifier::Bold));
			ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			const bool BodyOpen = ImGui::TreeNodeEx("Body", ImGuiTreeNodeFlags_SpanAvailWidth);
			ImGui::PopFont();
			if (BodyOpen) {
				Node_BodyEditor(*Actor, *Body, Edit);
				ImGui::TreePop();
			}
		}

		Node_Outline(*Actor);
	}

	void Actor::Entry(std::shared_ptr<CActor> Actor, std::shared_ptr<CScene> Scene, const std::size_t RowIndex)
	{
		LK_ASSERT(Actor && Scene);
		const LUUID Handle = Actor->GetHandle();
		ImGui::PushID(Handle);

		const bool IsSelected = CSelectionContext::IsSelected(Handle);
		ImGuiTreeNodeFlags TreeNodeFlags = ImGuiTreeNodeFlags_SpanAvailWidth
			| ImGuiTreeNodeFlags_AllowOverlap
			| ImGuiTreeNodeFlags_FramePadding
			| ImGuiTreeNodeFlags_OpenOnArrow;
		if (IsSelected) {
			TreeNodeFlags |= ImGuiTreeNodeFlags_Selected;
		}

		const ImGuiID ActorImGuiID = ImGui::GetID((void*)(std::uint64_t)(std::uint32_t)Handle);

		const ImVec2 RowMin = ImGui::GetCursorScreenPos();
		const ImVec2 RowAvail = ImGui::GetContentRegionAvail();
		const float RowHeight = ImGui::GetFrameHeight();
		if (!IsSelected && ((RowIndex & 1) != 0)) {
			ImGui::GetWindowDrawList()->AddRectFilled(
				RowMin, ImVec2(RowMin.x + RowAvail.x, RowMin.y + RowHeight), RGBA32::BackgroundDark);
		}

		bool NodeOpened = false;
		{
			UI::FScopedColorStack HeaderColors(
				ImGuiCol_Header, ImVec4(0.15f, 0.42f, 0.62f, 0.55f),
				ImGuiCol_HeaderHovered, ImVec4(0.20f, 0.50f, 0.72f, 0.55f),
				ImGuiCol_HeaderActive, ImVec4(0.20f, 0.50f, 0.72f, 0.75f));
			ImGui::SetNextItemStorageID(ActorImGuiID);
			NodeOpened = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<intptr_t>(ActorImGuiID)), TreeNodeFlags, "%s", "");
		}
		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
			CSelectionContext::Select(Handle);
		}

		{
			constexpr float IconSlot = 30.0f;
			ImGui::SameLine(0.0f, 2.0f);
			const float IconStartX = ImGui::GetCursorPosX();
			ImGui::AlignTextToFramePadding();
			ImGui::PushStyleColor(ImGuiCol_Text, IsSelected ? RGBA32::Text::Brighter : RGBA32::Text::Darker);
			ImGui::TextUnformatted(ActorTypeIcon(Actor->GetActorType()));
			ImGui::PopStyleColor();
			ImGui::SameLine();
			ImGui::SetCursorPosX(IconStartX + IconSlot);
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(Actor->GetName().data());
		}

		/* Delete button. */
		{
			const bool IsDeletable = Actor->IsDeletable();
			ImGui::SameLine(RowAvail.x - RowHeight);
			UI::FScopedColorStack TrashColors(
				ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f),
				ImGuiCol_ButtonHovered, ImVec4(0.55f, 0.12f, 0.12f, 0.85f),
				ImGuiCol_ButtonActive, ImVec4(0.70f, 0.15f, 0.15f, 1.0f));
			UI::FScopedColor TrashText(ImGuiCol_Text, IsDeletable ? RGBA32::Text::Darker : RGBA32::Text::Disabled);
			if (!IsDeletable) {
				ImGui::BeginDisabled();
			}
			if (ImGui::Button(LK_ICON_TRASH_O, ImVec2(RowHeight, RowHeight))) {
				LK_INFO_TAG("UI", "Delete: {} ({})", Handle, Actor->GetName());
				Scene->DeleteActor(Handle);
			}
			if (!IsDeletable) {
				ImGui::EndDisabled();
			}
			UI::SetTooltip(IsDeletable ? "Delete actor" : "Actor is not deletable");
		}

		if (NodeOpened) {
			Actor::Data(Actor);
			ImGui::TreePop();
		}

		ImGui::PopID();
	}

	void Actor::OnActorDeleted(const LUUID ActorHandle)
	{
		LK_DEBUG_TAG("UI", "Removing handle {} from actor cache", ActorHandle);
		ActorCache.Erase(ActorHandle);
		BodyEditCache.erase(ActorHandle);
	}

	static void AddComponentRow_Present(const char* Label)
	{
		ImGui::BeginDisabled();
		ImGui::TreeNodeEx(Label, ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth);
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::TextDisabled("(added)");
	}

	static void AddComponentForm_Transform(std::shared_ptr<CActor> Actor)
	{
		if (Actor->HasComponent<FTransformComponent>()) {
			AddComponentRow_Present("Transform");
			return;
		}

		if (ImGui::Selectable("Transform")) {
			Actor->AddComponent<FTransformComponent>();
			ImGui::CloseCurrentPopup();
		}
	}

	static void AddComponentForm_Health(std::shared_ptr<CActor> Actor)
	{
		if (Actor->HasComponent<FHealthComponent>()) {
			AddComponentRow_Present("Health");
			return;
		}

		if (!ImGui::TreeNodeEx("Health", ImGuiTreeNodeFlags_SpanAvailWidth)) {
			return;
		}

		static FHealthComponent Draft;
		BeginPropertyGrid(120.0f);
		Table::NextRow();
		UI::DragFloat("Max Health", Draft.MaxHealth, 1.0f, 1.0f, 10000.0f, "%.0f");
		Table::NextRow();
		UI::DragFloat("Health", Draft.Health, 1.0f, 0.0f, Draft.MaxHealth, "%.0f");
		Table::NextRow();
		bool Damageable = Draft.bDamageable;
		if (UI::Checkbox("Damageable", Damageable)) {
			Draft.bDamageable = Damageable;
		}
		EndPropertyGrid();

		if (ImGui::Button("Add Health", ImVec2(-1.0f, 0.0f))) {
			if (Draft.Health > Draft.MaxHealth) {
				Draft.Health = Draft.MaxHealth;
			}
			Actor->AddComponent<FHealthComponent>(Draft);
			Draft = FHealthComponent{};
			ImGui::CloseCurrentPopup();
		}

		ImGui::TreePop();
	}

	static void AddComponentForm_Interaction(std::shared_ptr<CActor> Actor)
	{
		if (Actor->HasComponent<FInteractionComponent>()) {
			AddComponentRow_Present("Interaction");
			return;
		}

		if (!ImGui::TreeNodeEx("Interaction", ImGuiTreeNodeFlags_SpanAvailWidth)) {
			return;
		}

		static FInteractionComponent Draft;
		static std::array<char, 64> CheckpointBuf = {0};

		BeginPropertyGrid(120.0f);
		Table::NextRow();
		Table::Label("Type");
		Table::NextColumn();
		ImGui::SetNextItemWidth(-1.0f);
		EInteraction Type = Draft.Type;
		if (UI::Combo("##InteractionType", Enum::View<EInteraction>(), Type)) {
			Draft.Type = Type;
			switch (Type) {
				case EInteraction::None:       Draft.Data = std::monostate{}; break;
				case EInteraction::Damage:     Draft.Data = FDamageInteraction{}; break;
				case EInteraction::Pickup:     Draft.Data = FPickupInteraction{}; break;
				case EInteraction::Heal:       Draft.Data = FHealInteraction{}; break;
				case EInteraction::Killzone:   Draft.Data = FKillzoneInteraction{}; break;
				case EInteraction::Jumppad:    Draft.Data = FJumppadInteraction{}; break;
				case EInteraction::Climbable:  Draft.Data = FClimbableInteraction{}; break;
				case EInteraction::Checkpoint: Draft.Data = FCheckpointInteraction{}; break;
				default:                       break;
			}
		}

		if (FDamageInteraction* D = std::get_if<FDamageInteraction>(&Draft.Data)) {
			Table::NextRow();
			UI::DragFloat("Damage", D->Damage, 1.0f, 0.0f, 1000.0f, "%.1f");
		} else if (FHealInteraction* H = std::get_if<FHealInteraction>(&Draft.Data)) {
			Table::NextRow();
			UI::DragFloat("Amount", H->Amount, 1.0f, 0.0f, 1000.0f, "%.1f");
			Table::NextRow();
			bool Consume = H->bConsumeOnUse;
			if (UI::Checkbox("Consume On Use", Consume)) {
				H->bConsumeOnUse = Consume;
			}
		} else if (FJumppadInteraction* J = std::get_if<FJumppadInteraction>(&Draft.Data)) {
			Table::NextRow();
			UI::DragFloat2("Impulse", J->Impulse, 0.0f, 0.10f, -50.0f, 50.0f);
			Table::NextRow();
			bool Preserve = J->bPreserveHorizontalVelocity;
			if (UI::Checkbox("Keep Horizontal", Preserve)) {
				J->bPreserveHorizontalVelocity = Preserve;
			}
		} else if (FClimbableInteraction* Climb = std::get_if<FClimbableInteraction>(&Draft.Data)) {
			Table::NextRow();
			UI::DragFloat("Climb Speed", Climb->ClimbSpeed, 0.10f, 0.0f, 50.0f, "%.2f");
		} else if (FCheckpointInteraction* Checkpoint = std::get_if<FCheckpointInteraction>(&Draft.Data)) {
			Table::NextRow();
			Table::Label("Checkpoint ID");
			Table::NextColumn();
			ImGui::SetNextItemWidth(-1.0f);
			if (ImGui::InputText("##CheckpointID", CheckpointBuf.data(), CheckpointBuf.size())) {
				Checkpoint->CheckpointID = CheckpointBuf.data();
			}
		}
		EndPropertyGrid();

		if (ImGui::Button("Add Interaction", ImVec2(-1.0f, 0.0f))) {
			Actor->AddComponent<FInteractionComponent>(Draft);
			Draft = FInteractionComponent{};
			CheckpointBuf.fill('\0');
			ImGui::CloseCurrentPopup();
		}

		ImGui::TreePop();
	}

	static void AddComponentForm_Effect(std::shared_ptr<CActor> Actor)
	{
		if (Actor->HasComponent<FEffectComponent>()) {
			AddComponentRow_Present("Effect");
			return;
		}

		if (!ImGui::TreeNodeEx("Effect", ImGuiTreeNodeFlags_SpanAvailWidth)) {
			return;
		}

		static float AngularSpeed = 90.0f;
		BeginPropertyGrid(120.0f);
		Table::NextRow();
		UI::DragFloat("Angular Speed", AngularSpeed, 1.0f, -720.0f, 720.0f, "%.0f");
		EndPropertyGrid();

		if (ImGui::Button("Add Effect", ImVec2(-1.0f, 0.0f))) {
			FEffectComponent EffectComp;
			FEffectInstance Instance;
			Instance.Type = EEffectType::Rotate;
			FRotateEffect Rotate;
			Rotate.AngularSpeedDegPerSecond = AngularSpeed;
			Instance.Data = Rotate;
			EffectComp.Effects.push_back(Instance);
			Actor->AddComponent<FEffectComponent>(EffectComp);
			ImGui::CloseCurrentPopup();
		}

		ImGui::TreePop();
	}

	void DrawComponents(std::shared_ptr<CActor> Actor)
	{
		if (!Actor) {
			return;
		}

		/***********************************
		 * Transform Component
		 ***********************************/
		DrawComponent<FTransformComponent>("Transform", Actor, [Actor](FTransformComponent& TC)
		{
			UI::BeginPropertyGrid();
			bool Changed = false;

			/* Translation */
			ImGui::TableNextRow();
			glm::vec3 Translation = TC.GetTranslation();
			if (UI::DragFloat3("Translation", Translation, 0.0f, 0.010f, -100.0f, 100.0f)) {
				Actor->SetPosition(Translation);
			}

			/* Rotation */
			ImGui::TableNextRow();
			float Rotation = glm::degrees(TC.GetRotation2D());
			if (UI::DragFloat("Rotation", Rotation, 0.10f, (-6 * 360.0f), (6 * 360.0f), "%.2f")) {
				Actor->SetRotation(glm::radians(Rotation));
			}

			/* Scale */
			ImGui::TableNextRow();
			glm::vec3 Scale = TC.GetScale();
			if (UI::DragFloat3("Scale", Scale, 1.0f, 0.010f, 0.010f)) {
				Actor->SetScale({Scale.x, Scale.y});
			}

			UI::EndPropertyGrid();
			ImGui::Dummy(ImVec2(0, 4));
		});

		/***********************************
		 * Health Component
		 **********************************/
		DrawComponent<FHealthComponent>("Health", Actor, [Actor](FHealthComponent& HC)
		{
			UI::BeginPropertyGrid();

			ImGui::TableNextRow();
			float Health = HC.GetHealth();
			if (UI::DragFloat("Health", Health, 1.0f, 1.0f, HC.GetMaxHealth(), "%1.f")) {
				HC.SetHealth(Health);
			}

			ImGui::TableNextRow();
			float MaxHealth = HC.GetMaxHealth();
			if (UI::DragFloat("Max Health", MaxHealth, 1.0f, 1.0f, 1000.0f, "%1.f")) {
				HC.SetMaxHealth(MaxHealth);
				/* Clamp the health if max health is less than current health. */
				if (MaxHealth < HC.GetHealth()) {
					HC.SetHealth(MaxHealth);
				}
			}

			ImGui::TableNextRow();
			bool bDamageable = HC.IsDamageable();
			if (UI::Checkbox("Damageable", bDamageable)) {
				HC.SetDamageable(bDamageable);
			}

			UI::EndPropertyGrid();
			ImGui::Dummy(ImVec2(0, 4));
		});

		/***********************************
		 * Interaction Component
		 **********************************/
		DrawComponent<FInteractionComponent>("Interaction", Actor, [Actor](FInteractionComponent& IC)
		{
			EInteraction InteractionType = IC.GetType();
			EInteraction Selected = InteractionType;
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 2));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

			const float ComboItemWidth = ((ImGui::GetContentRegionAvail().x) * 0.65f);
			ImGui::SetNextItemWidth(ComboItemWidth);
			const bool Updated = UI::Combo("##InteractionType", Enum::View<EInteraction>(), Selected);
			if (Updated) {
				InteractionType = Selected;
				IC.Type = InteractionType;
				LK_DEBUG_TAG("UI", "Set IC type: {}", Enum::ToString(IC.Type));

				/* Convert to new variant. */
				switch (IC.Type) {
					case EInteraction::None:
					{
						IC.Data = std::monostate{};
						break;
					}

					case EInteraction::Damage:
					{
						FDamageInteraction Data;
						IC.Data = Data;
						break;
					}

					case EInteraction::Pickup:
					{
						FPickupInteraction Data;
						IC.Data = Data;
						break;
					}

					case EInteraction::Heal:
					{
						FHealInteraction Data;
						IC.Data = Data;
						break;
					}

					case EInteraction::Killzone:
					{
						IC.Data = FKillzoneInteraction{};
						break;
					}

					case EInteraction::Jumppad:
					{
						FJumppadInteraction Data;
						IC.Data = Data;
						break;
					}

					case EInteraction::Climbable:
					{
						FClimbableInteraction Data;
						IC.Data = Data;
						break;
					}

					case EInteraction::Checkpoint:
					{
						FCheckpointInteraction Data;
						IC.Data = Data;
						break;
					}

					default:
						LK_ERROR_TAG("UI", "Unsupported interaction type: {}", Enum::ToString(IC.Type));
						break;
				}
			}
			ImGui::PopStyleVar(2);

			auto CheckboxInTable = [](std::string_view Label, bool& Value)
			{
				ImGui::PushID(Label.data());
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("%s", Label.data());

				ImGui::TableSetColumnIndex(1);
				ImGui::Checkbox("####Checkbox", &Value);
				ImGui::PopID();
			};

			/**********************************
			 * Interaction Data
			 **********************************/
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 2));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
			BeginPropertyGrid();
			switch (IC.GetType()) {
				case EInteraction::Damage:
				{
					ImGui::TableNextRow();
					auto& Data = std::get<FDamageInteraction>(IC.GetData());
					UI::DragFloat("Damage", Data.Damage, 1.0f, 0.0, 100.0f, "%.1f");
					break;
				}

				case EInteraction::Pickup:
				{
					auto& Data = std::get<FPickupInteraction>(IC.GetData());
					CheckboxInTable("Expire When Picked Up", Data.bExpireWhenPickedUp);

					EPickupKind SelectedKind = Data.Kind;
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("Kind");

					ImGui::TableSetColumnIndex(1);
					if (UI::Combo("##PickupKind", Enum::View<EPickupKind>(), SelectedKind)) {
						LK_DEBUG("Set pickup kind: {}", Enum::ToString(SelectedKind));
						Data.Kind = SelectedKind;
					}

					break;
				}

				case EInteraction::Heal:
				{
					ImGui::TableNextRow();
					auto& Data = std::get<FHealInteraction>(IC.GetData());
					UI::DragFloat("Amount", Data.Amount, 1.0f, 0.0f, 1000.0f, "%.1f");
					CheckboxInTable("Consume On Use", Data.bConsumeOnUse);
					break;
				}

				case EInteraction::Killzone:
				{
					break;
				}

				case EInteraction::Jumppad:
				{
					auto& Data = std::get<FJumppadInteraction>(IC.GetData());
					ImGui::TableNextRow();
					UI::DragFloat2("Impulse", Data.Impulse, 0.10f, -100.0f, 100.0f);
					CheckboxInTable("Preserve Horizontal Velocity", Data.bPreserveHorizontalVelocity);
					break;
				}

				case EInteraction::Climbable:
				{
					ImGui::TableNextRow();
					auto& Data = std::get<FClimbableInteraction>(IC.GetData());
					UI::DragFloat("Climb Speed", Data.ClimbSpeed, 0.10f, 0.0f, 20.0f, "%.2f");
					break;
				}

				case EInteraction::Checkpoint:
				{
					auto& Data = std::get<FCheckpointInteraction>(IC.GetData());
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("Checkpoint ID");
					ImGui::TableSetColumnIndex(1);

					std::array<char, 64> Buf{};
					const std::size_t CopyLen = std::min(Data.CheckpointID.size(), Buf.size() - 1);
					std::memcpy(Buf.data(), Data.CheckpointID.data(), CopyLen);
					if (ImGui::InputText("##CheckpointID", Buf.data(), Buf.size())) {
						Data.CheckpointID = Buf.data();
					}
					break;
				}

				default:
					break;
			}

			EndPropertyGrid();
			ImGui::PopStyleVar(2);
			ImGui::Dummy(ImVec2(0, 4));
		});

		/**********************************
		 * Effect Component
		 **********************************/
		DrawComponent<FEffectComponent>("Effect", Actor, [Actor](FEffectComponent& EC)
		{
			BeginPropertyGrid();
			for (auto& Effect : EC.Effects) {
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);

				switch (Effect.Type) {
					case EEffectType::Rotate:
					{
						ImGui::SetNextItemOpen(true, ImGuiCond_Appearing);
						if (ImGui::TreeNodeEx("Rotate", ImGuiTreeNodeFlags_SpanAllColumns)) {
							ImGui::TableNextRow();
							ImGui::TableSetColumnIndex(1);

							auto& EffectRef = std::get<FRotateEffect>(Effect.Data);
							UI::PushID();
							UI::DragFloat("Angular Speed (deg/s)", EffectRef.AngularSpeedDegPerSecond, 1.0f, -360.0f, 360.0f);
							UI::PopID();

							ImGui::TreePop();
						}
						break;
					}

					default:
						break;
				}
			}
			EndPropertyGrid();

			if (EC.HasAny()) {
				ImGui::Dummy(ImVec2(0, 10));
				ImGui::Separator();
				ImGui::Dummy(ImVec2(0, 10));
			}

			/*********************************
			 * Panel for adding effects.
			 *********************************/
			LargeTextCentralized("Add effects");
			static EEffectType EffectType = EEffectType::Rotate;
			static float AngularSpeed = 10.0f;

			BeginPropertyGrid();
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(1);

				static std::size_t SelectedIdx = std::to_underlying(EEffectType::Rotate);
				static constexpr std::array<EEffectType, std::to_underlying(EEffectType::COUNT)> EffectTypes = {
					EEffectType::None,
					EEffectType::Rotate,
				};

				static const auto Names = Enum::View<EEffectType, const char*>();

				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 2));
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
				const float ComboItemWidth = ((ImGui::GetContentRegionAvail().x) * 0.65f);
				ImGui::SetNextItemWidth(ComboItemWidth);
				if (ImGui::BeginCombo("##EffectType", Names[SelectedIdx])) {
					for (int Idx = 0; Idx < EffectTypes.size(); Idx++) {
						const bool IsSelected = (SelectedIdx == Idx);
						if (ImGui::Selectable(Names[Idx], IsSelected)) {
							SelectedIdx = Idx;
						}
					}

					ImGui::EndCombo();
					if (SelectedIdx != std::to_underlying(EffectType)) {
						EffectType = static_cast<EEffectType>(SelectedIdx);
					}
				}
				ImGui::PopStyleVar(2);

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				{
					UI::FScopedStyle ButtonRounding(ImGuiStyleVar_FrameRounding, 8.0f);
					switch (EffectType) {
						case EEffectType::Rotate:
							UI::DragFloat<EPlacementPolicy::Auto>("Angular Speed", AngularSpeed, 1.0f, -360.0f, 360.0f);
							break;

						default:
							break;
					}
				}
			}
			EndPropertyGrid();

			/***************************
			 * Button: Add
			 ***************************/
			static constexpr ImVec2 ButtonSize(92, 36);
			const ImVec2 Avail = ImGui::GetContentRegionAvail();
			ShiftCursorX(Avail.x - (ButtonSize.x + 10));
			FScopedStyle ButtonRounding(ImGuiStyleVar_FrameRounding, 8.0f);
			if (ImGui::Button("Add", ButtonSize)) {
				FEffectInstance Effect;
				Effect.Type = EffectType;
				switch (Effect.Type) {
					case EEffectType::Rotate:
					{
						LK_DEBUG_TAG("UI", "Add rotate effect: AngularSpeed={}", AngularSpeed);
						FRotateEffect Rotate;
						Rotate.AngularSpeedDegPerSecond = AngularSpeed;
						Effect.Data = Rotate;
						EC.Effects.push_back(Effect);
					}
				}
			}

			ImGui::Dummy(ImVec2(0, 4));
		});

		ImGui::Dummy(ImVec2(0, 16));

		/******************************
		 * Button: Add Component
		 ******************************/
		{
			UI::FScopedFont Font(UI::Font::Get(EFont::SourceSansPro, EFontSize::Large));
			UI::FScopedStyle ButtonRounding(ImGuiStyleVar_FrameRounding, 8.0f);
			UI::FScopedStyle ButtonPadding(ImGuiStyleVar_FramePadding, ImVec2(6, 6));
			UI::FScopedColorStack ButtonColors(
				ImGuiCol_ButtonHovered, RGBA32::NiceGreen,
				ImGuiCol_ButtonActive, RGBA32::LightGreen);

			std::array<char, 48> AddButtonLabel = {0};
			std::snprintf(AddButtonLabel.data(), AddButtonLabel.size(), "%s  Add Component", LK_ICON_PLUS);
			if (ImGui::Button(AddButtonLabel.data(), ImVec2(-1.0f, 0.0f))) {
				ImGui::OpenPopup("AddComponent");
			}
		}

		/******************************
		 * Popup: Add Component
		 ******************************/
		ImGui::SetNextWindowSize(ImVec2(300.0f, 0.0f), ImGuiCond_Appearing);
		if (ImGui::BeginPopup("AddComponent", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking)) {
			UI::FScopedStyle Rounding(ImGuiStyleVar_FrameRounding, 5.0f);

			AddComponentForm_Transform(Actor);
			AddComponentForm_Health(Actor);
			AddComponentForm_Interaction(Actor);
			AddComponentForm_Effect(Actor);

			ImGui::EndPopup();
		}
	}

	void DrawController(IController* Controller)
	{
		if (!Controller) {
			return;
		}
		if (Controller->GetControllerType() == EControllerType::Patrol) {
			auto* C = static_cast<CPatrolController*>(Controller);

			UI::BeginPropertyGrid();

			ImGui::TableNextRow();
			EDirection PatrolDir = C->GetPatrolDirection();
			if (UI::Combo("Patrol Direction", Enum::View<EDirection>(), PatrolDir)) {
				C->SetPatrolDirection(PatrolDir);
			}

			ImGui::TableNextRow();
			float DetectRadius = C->GetDetectRadius();
			if (UI::DragFloat("Detect Radius", DetectRadius, 0.010f, 0.0f, 5.0f)) {
				C->SetDetectRadius(DetectRadius);
			}

			ImGui::TableNextRow();
			float StopRadius = C->GetStopRadius();
			if (UI::DragFloat("Stop Radius", StopRadius, 0.010f, 0.0f, 5.0f)) {
				C->SetStopRadius(StopRadius);
			}

			ImGui::TableNextRow();
			const bool bHasTarget = C->HasTarget();
			bool bTargetPlayer = bHasTarget;
			if (UI::Checkbox("Target Player", bTargetPlayer)) {
				if (!bHasTarget && bTargetPlayer) {
					if (CGameInstance::IsValid()) {
						C->SetTarget(CGameInstance::Get().GetPlayer(0));
					}
				} else if (!bTargetPlayer) {
					C->SetTarget(nullptr);
				}
			}

			UI::EndPropertyGrid();
		}
	}

	void DrawEnemy(std::shared_ptr<CEnemy> Enemy)
	{
		if (!Enemy) {
			return;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 0));
		UI::BeginPropertyGrid();

		ImGui::TableNextRow();
		UI::Table::Label("Enemy State");
		UI::Table::NextColumn();
		EEnemyState EnemyState = Enemy->GetState();
		ImGui::SetNextItemWidth(200);
		if (UI::Combo("##EnemyState", Enum::View<EEnemyState>(), EnemyState)) {
			Enemy->SetState(EnemyState);
		}

		ImGui::Dummy(ImVec2(0, 4));
		ImGui::TableNextRow();
		FEnemyData& Data = Enemy->GetData();
		UI::DragFloat2("Spawn Point", Data.SpawnPoint, 0.0f, 0.010f);

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(1);
		UI::ShiftCursorX(7);
		const LUUID EnemyHandle = Enemy->GetHandle();
		bool ShowSpawnPoint = UI::IsSpawnPointVisible(EnemyHandle);
		if (ImGui::Checkbox("Show Spawn Point", &ShowSpawnPoint)) {
			UI::SetSpawnPointVisible(EnemyHandle, ShowSpawnPoint);
		}

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

		static constexpr ImVec2 ButtonSize(86, 32);
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(1);
		UI::ShiftCursorX(20.0f);
		bool EnemyDead = Enemy->IsDead();
		if (EnemyDead) {
			ImGui::BeginDisabled();
		}
		if (ImGui::Button("Kill", ButtonSize)) {
			Enemy->Kill();
		}
		if (EnemyDead) {
			ImGui::EndDisabled();
		}

		ImGui::SameLine(0.0f, 8.0f);
		EnemyDead = Enemy->IsDead();
		if (!EnemyDead) {
			ImGui::BeginDisabled();
		}
		if (ImGui::Button("Revive", ButtonSize)) {
			Enemy->Revive();
		}
		if (!EnemyDead) {
			ImGui::EndDisabled();
		}

		ImGui::PopStyleVar(1);
		ImGui::Dummy(ImVec2(0, 12));

		UI::EndPropertyGrid();
		ImGui::PopStyleVar(1);

		UI::ShiftCursorY(-12);
		IEnemyController* Controller = Enemy ? Enemy->GetController() : nullptr;
		UI::DrawController(Controller);
	}

	void Rifle(std::shared_ptr<CRifle> InRifle)
	{
		if (!InRifle) {
			return;
		}

		FScopedFont Font(EFont::SourceSansPro, EFontSize::Large);
		ImGui::SetNextItemOpen(true, ImGuiCond_Once);
		if (ImGui::TreeNodeEx("Rifle", ImGuiTreeNodeFlags_SpanAvailWidth)) {
			UI::RifleData(InRifle);
			ImGui::TreePop();
		}
	}

	void SceneManagerPanel(std::shared_ptr<CScene> InScene)
	{
		const bool WindowOpened = UI::Begin(PanelID::SceneManager, nullptr);
		if (!WindowOpened) {
			return;
		}
		if (!InScene) {
			UI::End();
			return;
		}

		const auto& Actors = InScene->GetActors();
		std::shared_ptr<CPlayer> Player = CGameInstance::Get().GetPlayer(0);
		const std::size_t ActorCount = Actors.size() + (Player ? 1 : 0);

		UI::Font::Push(EFont::SourceSansPro, EFontSize::Large);
		ImGui::AlignTextToFramePadding();
		ImGui::Text("%s   %s", LK_ICON_DESKTOP, InScene->GetName().data());
		UI::SetTooltip("%s actors", ActorCount);
		UI::Font::Pop();

		/* Search / filter. */
		static std::array<char, 96> SearchBuf = {0};
		{
			UI::FScopedStyle Rounding(ImGuiStyleVar_FrameRounding, 6.0f);
			std::array<char, 80> Hint = {0};
			std::snprintf(Hint.data(), Hint.size(), "%s  Search", LK_ICON_SEARCH);
			ImGui::SetNextItemWidth(-1.0f);
			ImGui::InputTextWithHint("##ActorSearch", Hint.data(), SearchBuf.data(), SearchBuf.size());
		}

		ImGui::Dummy(ImVec2(0, 4));

		const std::string_view Filter(SearchBuf.data());

		std::size_t RowIndex = 0;
		if (Player && ContainsCI(Player->GetName(), Filter)) {
			UI::Actor::Entry(Player, InScene, RowIndex++);
		}
		for (auto& Actor : Actors) {
			if (!ContainsCI(Actor->GetName(), Filter)) {
				continue;
			}
			UI::Actor::Entry(Actor, InScene, RowIndex++);
		}

		UI::End();
	}

	void EditorViewportInfo(bool Focused, bool Hovered)
	{
		ImGui::SeparatorText("Editor Viewport");
		ImGui::Text("Focused: %s", Focused ? "Yes" : "No");
		ImGui::Text("Hovered: %s", Hovered ? "Yes" : "No");
	}
}
