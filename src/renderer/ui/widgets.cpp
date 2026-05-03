#include "widgets.h"

#include "core/window.h"
#include "core/input/keyboard.h"
#include "game/instance.h"
#include "core/selectioncontext.h"
#include "game/spawner.h"
#include "game/controller/patrolcontroller.h"
#include "renderer/ui/editor_resources.h"
#include "renderer/ui/imgui.h"
#include "ui.h"
#include "scene/scene.h"

namespace platformer2d::UI::Widget {

	struct FActorDataEntry
	{
		std::array<char, 64> NameBuf;
	};

	namespace {
		std::unordered_map<LUUID, FActorDataEntry> ActorDataMap;
	}

	void ActorNode::Buttons(std::shared_ptr<CActor> Actor, std::shared_ptr<CScene> Scene)
	{
		LK_ASSERT(Actor && Scene);
		if (!Actor || !Scene) {
			return;
		}

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

	void ActorNode::DeleteButton(std::shared_ptr<CActor> Actor, std::shared_ptr<CScene> Scene)
	{
		LK_ASSERT(Actor && Scene);
		if (!Actor || !Scene) {
			return;
		}

		static const ImVec2 Avail = ImGui::GetContentRegionAvail();
		static constexpr ImVec2 ButtonSize = ImVec2(138, 50);

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

	void ActorNode::Data(std::shared_ptr<CActor> Actor)
	{
		const LUUID Handle = Actor->GetHandle();
		ImGui::PushID(Handle);

		static constexpr float LabelColumnWidth = 180.0f;
		ImGui::BeginTable("##ActorNode_Data", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip);
		ImGui::TableSetupColumn("L", 0, LabelColumnWidth);
		ImGui::TableSetupColumn("V", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, ImGui::GetContentRegionAvail().x - LabelColumnWidth);

		/**
		 * Cache the actor, done to be able to rename it.
		 */
		if (!ActorDataMap.contains(Handle)) {
			LK_TRACE_TAG("UI", "Caching handle {}", Handle);
			auto [Iter, Inserted] = ActorDataMap.emplace(Handle, FActorDataEntry{});
			if (Inserted) {
				/* Populate the buffer with the actor name. */
				FActorDataEntry& Data = Iter->second;
				std::snprintf(Data.NameBuf.data(), Data.NameBuf.size(), "%s", Actor->GetName().data());
			}
		}

		FTransformComponent& TC = Actor->GetTransformComponent();
		const CBody* Body = Actor->GetBody();

		/* Actor name. */
		auto Iter = ActorDataMap.find(Handle);
		if (Iter != ActorDataMap.end()) {
			ImGui::TableNextRow();
			FActorDataEntry& Data = Iter->second;
			UI::Table::Label("Name");
			UI::Table::NextColumn();

			UI::FScopedStyle ButtonFrame(ImGuiStyleVar_FramePadding, ImVec2(5, 5));
			ImGui::InputText("##Name", Data.NameBuf.data(), Data.NameBuf.size());
			ImGui::SameLine();

			UI::FScopedFont Font(UI::Font::Get(EFont::SourceSansPro, EFontSize::Regular, EFontModifier::Bold));
			UI::FScopedStyle ButtonRounding(ImGuiStyleVar_FrameRounding, 3);
			if (ImGui::Button(LK_ICON_CHECK_CIRCLE)) {
				LK_DEBUG_TAG("UI", "Rename {} to: {}", Handle, Data.NameBuf.data());
				Actor->SetName(Data.NameBuf.data());
			}
		}

		/* Texture. */
		Table::NextRow();
		{
			ETexture Texture = Actor->GetTexture();
			if (UI::Widget::Combo::TextureDropdown(Texture)) {
				LK_INFO_TAG("UI", "Update {} texture: {}", Actor->GetName(), Enum::ToString(Texture));
				Actor->SetTexture(Texture);
			}
		}

		/* Color. */
		Table::NextRow();
		{
			const glm::vec4& ColorRef = Actor->GetColor();
			EColor Color = EColor::White;
			const bool ColorDeduced = FColor::DeduceEnum(Color, ColorRef);
			if (!ColorDeduced) {
				ImGui::BeginDisabled();
			}
			if (ColorDropdown(Color)) {
				LK_INFO_TAG("UI", "Update {} color: {}", Actor->GetName(), Enum::ToString(Color));
				Actor->SetColor(FColor::Get(Color));
			}
			if (!ColorDeduced) {
				ImGui::EndDisabled();
			}
		}

		/* Tick info. */
		UI::Table::NextRow();
		{
			Table::Label("Tick");
			Table::NextColumn();
			ImGui::Text("%s", Actor->IsTickEnabled() ? "Enabled" : "Disabled");
		}

		if (Body) {
			/* Body Type. */
			ImGui::TableNextRow();
			{
				Table::Label("Body Type");
				Table::NextColumn();
				ImGui::Text("%s", Enum::ToString(Body->GetType()));
			}

			/* Body Size. */
			ImGui::TableNextRow();
			{
				Table::Label("Body Size");
				Table::NextColumn();
				const glm::vec2 Size = Body->GetSize();
				ImGui::Text("(%.2f, %.2f)", Size.x, Size.y);
			}

			/* Linear Velocity. */
			Table::NextRow();
			{
				Table::Label("Linear Velocity");
				Table::NextColumn();
				const glm::vec2 V = Body->GetLinearVelocity();
				ImGui::Text("(%.2f, %.2f)", V.x, V.y);
			}

			/* Angular Velocity. */
			Table::NextRow();
			{
				Table::Label("Angular Velocity");
				Table::NextColumn();
				const float V = Body->GetAngularVelocity();
				ImGui::Text("%.2f", V);
			}

			/* AABB. */
			Table::NextRow();
			{
				Table::Label("AABB");
				Table::NextColumn();
				const FAABB AABB = Body->GetAABB();
				ImGui::Text("Min (%.2f, %.2f)", AABB.Min.x, AABB.Min.y);
				ImGui::SameLine(0.0f, 16.0f);
				ImGui::Text("Max (%.2f, %.2f)", AABB.Max.x, AABB.Max.y);
			}

			/* Enabled. */
			ImGui::TableNextRow();
			{
				bool Enabled = Body->IsEnabled();
				if (UI::Checkbox("Body Enabled", Enabled)) {
					Body->SetEnabled(Enabled);
				}
			}

			/* Awake. */
			ImGui::TableNextRow();
			{
				Table::Label("Awake");
				Table::NextColumn();
				ImGui::Text("%s", Body->IsAwake() ? "Yes" : "No");
			}

			/* Sensor. */
			ImGui::TableNextRow();
			{
				Table::Label("Sensor");
				Table::NextColumn();
				ImGui::Text("%s", Body->IsSensor() ? "Yes" : "No");
			}
		}

		ImGui::EndTable();

		/* Node: Outline */
		if (ImGui::TreeNodeEx("Outline", ImGuiTreeNodeFlags_SpanAvailWidth)) {
			UI::BeginPropertyGrid();

			/* Outline enabled. */
			Table::NextRow();
			{
				UI::Table::Label("Enabled");
				UI::Table::NextColumn();
				bool Enabled = Actor->IsOutlineEnabled();
				if (ImGui::Checkbox("##Enabled", &Enabled)) {
					Actor->SetOutlineEnabled(Enabled);
				}
			}

			/* Outline thickness. */
			Table::NextRow();
			{
				float Thickness = Actor->GetOutlineThickness();
				if (UI::Widget::DragFloat("Thickness", Thickness, 0.10f, 0.0f, 20.0f)) {
					Actor->SetOutlineThickness(Thickness);
				}
			}

			/* Outline color. */
			Table::NextRow();
			{
				const glm::vec4& Color = Actor->GetOutlineColor();
				glm::vec3 C = {Color.x, Color.y, Color.z};
				if (UI::Widget::DragFloat3("Outline Color", C, 1.0f, 0.010f, 0.0f, 1.0f)) {
					Actor->SetOutlineColor(glm::vec4(C, 1.0f));
				}
			}
			UI::EndPropertyGrid();

			ImGui::TreePop();
		}

		ImGui::PopID();
	}

	void ActorNode::Entry(std::shared_ptr<CActor> Actor, std::shared_ptr<CScene> Scene)
	{
		LK_ASSERT(Actor && Scene);
		const LUUID Handle = Actor->GetHandle();

		const bool IsSelected = CSelectionContext::IsSelected(Handle);
		ImGuiTreeNodeFlags TreeNodeFlags = (IsSelected ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None);
		TreeNodeFlags |= ImGuiTreeNodeFlags_SpanAvailWidth;

		const ImGuiID ActorImGuiID = ImGui::GetID((void*)(std::uint64_t)(std::uint32_t)Handle);
		std::string_view Name = Actor->GetName();
		std::array<char, 84> NodeName = {0};
		std::snprintf(NodeName.data(), NodeName.size(), "%s", Name.data());

		const bool WasOpen = ImGui::TreeNodeBehaviorIsOpen(ActorImGuiID);
		ImGui::SetNextItemStorageID(ActorImGuiID);
		const bool NodeOpened = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<intptr_t>(ActorImGuiID)), TreeNodeFlags, NodeName.data());
		ImGui::PushID(Handle);
		if (NodeOpened) {
			ActorNode::Data(Actor);
			UI::Widget::DrawComponents(Actor);
			UI::Widget::ActorNode::DeleteButton(Actor, Scene);
			ImGui::TreePop();

			if (!WasOpen && !CSelectionContext::IsAnySelected()) {
				LK_DEBUG_TAG("UI", "Selecting: {} ({})", NodeName.data(), Handle);
				CSelectionContext::Select(Handle);
			}
		}

		ImGui::PopID();
	}

	void OnActorDeleted(const LUUID ActorHandle)
	{
		LK_DEBUG_TAG("UI", "Removing handle {} from actor cache", ActorHandle);
		ActorDataMap.erase(ActorHandle);
	}

	void DrawComponents(std::shared_ptr<CActor> Actor)
	{
		if (!Actor) {
			return;
		}

		/***********************************
		 * Transform Component
		 **********************************/
		UI::Widget::DrawComponent<FTransformComponent>("Transform", Actor, [Actor](FTransformComponent& TC)
		{
			UI::BeginPropertyGrid();
			bool Changed = false;

			/* Translation */
			ImGui::TableNextRow();
			glm::vec3 Translation = TC.GetTranslation();
			if (UI::Widget::DragFloat3("Translation", Translation, 0.0f, 0.010f, -100.0f, 100.0f)) {
				Actor->SetPosition(Translation);
			}

			/* Rotation */
			ImGui::TableNextRow();
			float Rotation = glm::degrees(TC.GetRotation2D());
			if (UI::Widget::DragFloat("Rotation", Rotation, 0.10f, (-6 * 360.0f), (6 * 360.0f), "%.2f")) {
				Actor->SetRotation(glm::radians(Rotation));
			}

#if 0 /* SCALING NEEDS TO BE SUPPORTED */
			/* Scale */
			ImGui::TableNextRow();
			glm::vec3& Scale = TC.Scale;
			UI::Widget::DragFloat3("Scale", Scale, 1.0f, 0.010f, 0.010f);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			if (ImGui::Button("Rebuild")) {
				Actor->GetBody()->Rebuild();
			}
#endif

			UI::EndPropertyGrid();
			ImGui::Dummy(ImVec2(0, 4));
		});

		/***********************************
		 * Health Component
		 **********************************/
		UI::Widget::DrawComponent<FHealthComponent>("Health", Actor, [Actor](FHealthComponent& HC)
		{
			UI::BeginPropertyGrid();

			ImGui::TableNextRow();
			float Health = HC.GetHealth();
			if (UI::Widget::DragFloat("Health", Health, 1.0f, 1.0f, HC.GetMaxHealth(), "%1.f")) {
				HC.SetHealth(Health);
			}

			ImGui::TableNextRow();
			float MaxHealth = HC.GetMaxHealth();
			if (UI::Widget::DragFloat("Max Health", MaxHealth, 1.0f, 1.0f, 1000.0f, "%1.f")) {
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
		UI::Widget::DrawComponent<FInteractionComponent>("Interaction", Actor, [Actor](FInteractionComponent& IC)
		{
			EInteraction InteractionType = IC.GetType();
			std::size_t SelectedIdx = std::to_underlying(InteractionType);
			static constexpr std::array<EInteraction, std::to_underlying(EInteraction::COUNT)> Types = {
				EInteraction::None,
				EInteraction::Damage,
				EInteraction::Pickup,
			};

			bool Updated = false;
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 2));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
			const float ComboItemWidth = ((ImGui::GetContentRegionAvail().x) * 0.65f);
			ImGui::SetNextItemWidth(ComboItemWidth);
			if (ImGui::BeginCombo("##EffectType", Enum::ToString(InteractionType))) {
				for (int Idx = 0; Idx < Types.size(); Idx++) {
					const char* Option = Enum::ToString(Types[Idx]);
					if (Option == nullptr) {
						continue;
					}

					const bool IsSelected = (SelectedIdx == Idx);
					if (ImGui::Selectable(Option, IsSelected)) {
						if (SelectedIdx != Idx) {
							Updated = true;
							SelectedIdx = Idx;
						}
					}
				}

				ImGui::EndCombo();
				if (Updated) {
					InteractionType = static_cast<EInteraction>(SelectedIdx);
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

						default:
							LK_ERROR_TAG("UI", "Unsupported interaction type: {}", Enum::ToString(IC.Type));
							break;
					}
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
			UI::BeginPropertyGrid();
			switch (IC.GetType()) {
				case EInteraction::Damage:
				{
					ImGui::TableNextRow();
					auto& Data = std::get<FDamageInteraction>(IC.GetData());
					UI::Widget::DragFloat("Damage", Data.Damage, 1.0f, 0.0, 100.0f, "%.1f");
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
					if (UI::Combo("##PickupKind", Array::PickupKind, SelectedKind)) {
						LK_DEBUG("Set pickup kind: {}", Enum::ToString(SelectedKind));
						Data.Kind = SelectedKind;
					}

					break;
				}

				default:
					break;
			}

			UI::EndPropertyGrid();
			ImGui::PopStyleVar(2);
			ImGui::Dummy(ImVec2(0, 4));
		});

		/**********************************
		 * Effect Component
		 **********************************/
		UI::Widget::DrawComponent<FEffectComponent>("Effect", Actor, [Actor](FEffectComponent& EC)
		{
			UI::BeginPropertyGrid();
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
							UI::Widget::DragFloat("Angular Speed (deg/s)", EffectRef.AngularSpeedDegPerSecond, 1.0f, -360.0f, 360.0f);
							UI::PopID();

							ImGui::TreePop();
						}
						break;
					}

					default:
						break;
				}
			}
			UI::EndPropertyGrid();

			if (EC.HasAny()) {
				ImGui::Dummy(ImVec2(0, 10));
				ImGui::Separator();
				ImGui::Dummy(ImVec2(0, 10));
			}

			/*********************************
			 * Panel for adding effects.
			 *********************************/
			UI::LargeTextCentralized("Add effects");
			static EEffectType EffectType = EEffectType::Rotate;
			static float AngularSpeed = 10.0f;

			UI::BeginPropertyGrid();
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(1);

				static std::size_t SelectedIdx = std::to_underlying(EEffectType::Rotate);
				static constexpr std::array<EEffectType, std::to_underlying(EEffectType::COUNT)> EffectTypes = {
					EEffectType::None,
					EEffectType::Rotate,
				};

				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 2));
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
				const float ComboItemWidth = ((ImGui::GetContentRegionAvail().x) * 0.65f);
				ImGui::SetNextItemWidth(ComboItemWidth);
				if (ImGui::BeginCombo("##EffectType", Enum::ToString(EffectType))) {
					for (int Idx = 0; Idx < EffectTypes.size(); Idx++) {
						const char* Option = Enum::ToString(EffectTypes[Idx]);
						if (Option == nullptr) {
							continue;
						}

						const bool IsSelected = (SelectedIdx == Idx);
						if (ImGui::Selectable(Option, IsSelected)) {
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
							UI::Widget::DragFloat<EPlacementPolicy::Auto>("Angular Speed", AngularSpeed, 1.0f, -360.0f, 360.0f);
							break;

						default:
							break;
					}
				}
			}
			UI::EndPropertyGrid();

			/***************************
			 * Button: Add
			 ***************************/
			static constexpr ImVec2 ButtonSize(92, 36);
			const ImVec2 Avail = ImGui::GetContentRegionAvail();
			UI::ShiftCursorX(Avail.x - (ButtonSize.x + 10));
			UI::FScopedStyle ButtonRounding(ImGuiStyleVar_FrameRounding, 8.0f);
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

		ImGui::Dummy(ImVec2(0, 30));

		/******************************
		 * Button: Add Component
		 ******************************/
		{
			UI::FScopedFont Font(UI::Font::Get(EFont::SourceSansPro, EFontSize::Large));
			UI::FScopedStyle ButtonRounding(ImGuiStyleVar_FrameRounding, 8.0f);

			const float LineHeight = GImGui->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			static const char* AddButtonLabel = "Add Component";
			ImVec2 AddTextSize = ImGui::CalcTextSize(AddButtonLabel);
			ImVec2 ButtonSize = ImVec2(AddTextSize.x * 1.30f, LineHeight + 2.0f);

			const ImVec2 Avail = ImGui::GetContentRegionAvail();
			UI::ShiftCursorX(Avail.x - (ButtonSize.x + 10));
			if (ImGui::Button(AddButtonLabel, ButtonSize)) {
				ImGui::OpenPopup("AddComponent");
			}
		}

		/******************************
		 * Popup: Add Component
		 ******************************/
		if (ImGui::BeginPopup("AddComponent", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking)) {
			static constexpr float AddCompPanelWidth = 250.0f;
			if (ImGui::BeginTable("##CompTable", 2, ImGuiTableFlags_SizingStretchSame)) {
				ImGui::TableSetupColumn("Icon", ImGuiTableColumnFlags_WidthFixed, AddCompPanelWidth * 0.12f);
				ImGui::TableSetupColumn("Components", ImGuiTableColumnFlags_WidthFixed, AddCompPanelWidth * 0.88f);

				UI::Widget::DrawAddComponentButton<FTransformComponent>("Transform", EditorResources.PlusIcon, Actor);
				UI::Widget::DrawAddComponentButton<FEffectComponent>("Effect", EditorResources.PlusIcon, Actor);
				UI::Widget::DrawAddComponentButton<FInteractionComponent>("Interaction", EditorResources.PlusIcon, Actor);
				UI::Widget::DrawAddComponentButton<FHealthComponent>("Health", EditorResources.PlusIcon, Actor);

				ImGui::EndTable();
			}

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
			if (UI::Combo("Patrol Direction", UI::Array::Direction, PatrolDir)) {
				C->SetPatrolDirection(PatrolDir);
			}

			ImGui::TableNextRow();
			float DetectRadius = C->GetDetectRadius();
			if (UI::Widget::DragFloat("Detect Radius", DetectRadius, 0.010f, 0.0f, 5.0f)) {
				C->SetDetectRadius(DetectRadius);
			}

			ImGui::TableNextRow();
			float StopRadius = C->GetStopRadius();
			if (UI::Widget::DragFloat("Stop Radius", StopRadius, 0.010f, 0.0f, 5.0f)) {
				C->SetStopRadius(StopRadius);
			}

			ImGui::TableNextRow();
			const bool bHasTarget = C->HasTarget();
			bool bTargetPlayer = bHasTarget;
			if (UI::Checkbox("Target Player", bTargetPlayer)) {
				if (!bHasTarget && bTargetPlayer) {
					if (CGameInstance* GameInstance = CGameInstance::Get()) {
						C->SetTarget(GameInstance->GetPlayer(0));
					}
				} else if (!bTargetPlayer) {
					C->SetTarget(nullptr);
				}
			}

			ImGui::TableNextRow();
			UI::Table::Label("Has Target");
			UI::Table::NextColumn();
			ImGui::Text("%s", (C->HasTarget() ? "Yes" : "No"));

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
		if (UI::Combo("##EnemyState", UI::Array::EnemyStates, EnemyState)) {
			Enemy->SetState(EnemyState);
		}

		ImGui::Dummy(ImVec2(0, 4));
		ImGui::TableNextRow();
		FEnemyData& Data = Enemy->GetData();
		UI::Widget::DragFloat2("Spawn Point", Data.SpawnPoint, 0.0f, 0.010f);

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
		UI::Widget::DrawController(Controller);
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
		if (!InScene) {
			return;
		}

		UI::HeaderTextCentralized("Scene");
		const auto Actors = InScene->GetActors();

		UI::Font::Push(EFont::SourceSansPro, EFontSize::Large);
		if (UI::BeginPropertyGrid(80)) {
			Table::NextRow();
			UI::Table::Label("Actors");
			UI::Table::NextColumn();
			ImGui::Text("%d", Actors.size() + 1); /* +1 for player */
			UI::EndPropertyGrid();
		}
		UI::Font::Pop();

		ImGui::Dummy(ImVec2(0, 4));

		if (std::shared_ptr<CPlayer> Player = CGameInstance::Get()->GetPlayer(0)) {
			UI::Widget::ActorNode::Entry(Player, InScene);
		}
		for (auto& Actor : Actors) {
			UI::Widget::ActorNode::Entry(Actor, InScene);
		}
	}

	void EditorViewportInfo(bool Focused, bool Hovered)
	{
		ImGui::SeparatorText("Editor Viewport");
		ImGui::Text("Focused: %s", Focused ? "Yes" : "No");
		ImGui::Text("Hovered: %s", Hovered ? "Yes" : "No");
	}

}
