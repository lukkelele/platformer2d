#include "widgets.h"

#include "core/window.h"
#include "core/input/keyboard.h"
#include "game/instance.h"
#include "core/selectioncontext.h"
#include "game/spawner.h"
#include "renderer/imgui.h"
#include "renderer/ui/editor_resources.h"
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

	void ActorNode_Data(std::shared_ptr<CActor> Actor)
	{
		const LUUID Handle = Actor->GetHandle();
		ImGui::PushID(Handle);

		static constexpr float LabelColumnWidth = 180.0f;
		ImGui::BeginTable("##ActorNode_Data", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoClip);
		ImGui::TableSetupColumn("L", 0, LabelColumnWidth);
		ImGui::TableSetupColumn("V", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, ImGui::GetContentRegionAvail().x - LabelColumnWidth);

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
			UI::ShiftCursor(0.0f, 4.0f);
		};

		/* Cache the actor. */
		if (!ActorDataMap.contains(Handle))
		{
			LK_TRACE_TAG("UI", "Caching handle {}", Handle);
			auto [Iter, Inserted] = ActorDataMap.emplace(Handle, FActorDataEntry{});
			if (Inserted)
			{
				/* Populate the buffer with the actor name. */
				FActorDataEntry& Data = Iter->second;
				std::snprintf(Data.NameBuf.data(), Data.NameBuf.size(), "%s", Actor->GetName().data());
			}
		}

		FTransformComponent& TC = Actor->GetTransformComponent();
		const CBody& Body = Actor->GetBody();

		/* Actor name */
		auto Iter = ActorDataMap.find(Handle);
		if (Iter != ActorDataMap.end())
		{
			ImGui::TableNextRow();
			FActorDataEntry& Data = Iter->second;
			Label("Name");
			NextColumn();
			UI::ShiftCursor(7.0f, 0.0f);

			UI::FScopedStyle ButtonFrame(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
			ImGui::InputText("##Name", Data.NameBuf.data(), Data.NameBuf.size());
			ImGui::SameLine();

			UI::FScopedFont Font(UI::Font::Get(EFont::SourceSansPro, EFontSize::Regular, EFontModifier::Bold));
			UI::FScopedStyle ButtonRounding(ImGuiStyleVar_FrameRounding, 2);
			if (ImGui::Button(LK_ICON_CHECK_CIRCLE))
			{
				LK_DEBUG_TAG("UI", "Rename {} to: {}", Handle, Data.NameBuf.data());
				Actor->SetName(Data.NameBuf.data());
			}
		}

		/* Texture */
		ImGui::TableNextRow();
		{
			ETexture Texture = Actor->GetTexture();
			if (TextureDropdown(Texture))
			{
				LK_INFO_TAG("UI", "Update {} texture: {}", Actor->GetName(), Enum::ToString(Texture));
				Actor->SetTexture(Texture);
			}
		}

		/* Color */
		ImGui::TableNextRow();
		{
			const glm::vec4& ColorRef = Actor->GetColor();
			EColor Color = EColor::White;
			const bool ColorDeduced = FColor::DeduceEnum(Color, ColorRef);
			if (!ColorDeduced)
			{
				ImGui::BeginDisabled();
			}
			if (ColorDropdown(Color))
			{
				LK_INFO_TAG("UI", "Update {} color: {}", Actor->GetName(), Enum::ToString(Color));
				Actor->SetColor(FColor::Get(Color));
			}
			if (!ColorDeduced)
			{
				ImGui::EndDisabled();
			}
		}

		/* Outline thickness */
		ImGui::TableNextRow();
		{
			float Thickness = Actor->GetOutlineThickness();
			if (UI::Widget::DragFloat("Outline Thickness", Thickness, 0.10f, 0.0f, 20.0f))
			{
				Actor->SetOutlineThickness(Thickness);
			}
		}

		/* Outline color */
		ImGui::TableNextRow();
		{
			const glm::vec4& Color = Actor->GetOutlineColor();
			glm::vec3 C = { Color.x, Color.y, Color.z };
			if (UI::Widget::Vec3Control("Outline Color", C, 1.0f, 0.010f, 0.0f, 1.0f))
			{
				Actor->SetOutlineColor(glm::vec4(C, 1.0f));
			}
		}

		/* Tick info. */
		ImGui::TableNextRow();
		{
			Label("Tick");
			NextColumn();
			ImGui::Text("%s", Actor->IsTickEnabled() ? "Enabled" : "Disabled");
		}

		/* Body Type. */
		ImGui::TableNextRow();
		{
			Label("Body Type");
			NextColumn();
			ImGui::Text("%s", Enum::ToString(Body.GetType()));
		}

		/* Body Size. */
		ImGui::TableNextRow();
		{
			Label("Body Size");
			NextColumn();
			const glm::vec2 Size = Body.GetSize();
			ImGui::Text("(%.2f, %.2f)", Size.x, Size.y);
		}

		/* Linear Velocity. */
		ImGui::TableNextRow();
		{
			Label("Linear Velocity");
			NextColumn();
			const glm::vec2 V = Body.GetLinearVelocity();
			ImGui::Text("(%.2f, %.2f)", V.x, V.y);
		}

		/* Angular Velocity. */
		ImGui::TableNextRow();
		{
			Label("Angular Velocity");
			NextColumn();
			const float V = Body.GetAngularVelocity();
			ImGui::Text("%.2f", V);
		}

		/* AABB. */
		ImGui::TableNextRow();
		{
			Label("AABB");
			NextColumn();
			const FAABB AABB = Body.GetAABB();
			ImGui::Text("Min (%.2f, %.2f)", AABB.Min.x, AABB.Min.y);
			ImGui::SameLine(0.0f, 16.0f);
			ImGui::Text("Max (%.2f, %.2f)", AABB.Max.x, AABB.Max.y);
		}

		/* Awake. */
		ImGui::TableNextRow();
		{
			Label("Awake");
			NextColumn();
			ImGui::Text("%s", Body.IsAwake() ? "Yes" : "No");
		}

		/* Sensor. */
		ImGui::TableNextRow();
		{
			Label("Sensor");
			NextColumn();
			ImGui::Text("%s", Body.IsSensor() ? "Yes" : "No");
		}

		ImGui::EndTable();

		ImGui::PopID();
	}

	void ActorNode_Buttons(std::shared_ptr<CActor> Actor, std::shared_ptr<CScene> Scene)
	{
		LK_ASSERT(Actor && Scene);
		if (!Actor || !Scene)
		{
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
					ImGuiCol_ButtonActive, RGBA32::NiceGreen
				);

				const bool ActorExists = Scene->DoesActorExist(ActorAttr.NameBuf.data());
				if (ActorExists)
				{
					ImGui::BeginDisabled();
				}

				const float CursorPosX = 0.50f * ImGui::GetContentRegionAvail().x;
				UI::ShiftCursorX(CursorPosX - ButtonSize.x);
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

				UI::ShiftCursorX(ImGui::GetStyle().FramePadding.x);
				const bool IsDeletable = Actor->IsDeletable();
				if (!IsDeletable)
				{
					ImGui::BeginDisabled();
				}
				if (ImGui::Button("Delete", ButtonSize))
				{
					const LUUID ActorHandle = Actor->GetHandle();
					LK_INFO("Delete: {} ({})", ActorHandle, Actor->GetName());
					Scene->DeleteActor(ActorHandle);
				}
				if (!IsDeletable)
				{
					ImGui::EndDisabled();
				}
			}

			ImGui::EndTable();
		}
	}

	void ActorDeleteButton(std::shared_ptr<CActor> Actor, std::shared_ptr<CScene> Scene)
	{
		LK_ASSERT(Actor && Scene);
		if (!Actor || !Scene)
		{
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
			ImGuiCol_ButtonActive, RGBA32::Red
		);

		UI::ShiftCursorX(ImGui::GetStyle().FramePadding.x);
		const bool IsDeletable = Actor->IsDeletable();
		if (!IsDeletable)
		{
			ImGui::BeginDisabled();
		}
		if (ImGui::Button("Delete Actor", ButtonSize))
		{
			const LUUID ActorHandle = Actor->GetHandle();
			LK_INFO("Delete: {} ({})", ActorHandle, Actor->GetName());
			Scene->DeleteActor(ActorHandle);
		}
		if (!IsDeletable)
		{
			ImGui::EndDisabled();
		}
	}

	void ActorNode(std::shared_ptr<CActor> Actor, std::shared_ptr<CScene> Scene)
	{
		LK_ASSERT(Actor && Scene);
		const LUUID Handle = Actor->GetHandle();
		ImGui::PushID(Handle);

		const bool IsSelected = CSelectionContext::IsSelected(Handle);

		ImGuiTreeNodeFlags TreeNodeFlags = (IsSelected ? ImGuiTreeNodeFlags_Selected : ImGuiTreeNodeFlags_None);
		TreeNodeFlags |= ImGuiTreeNodeFlags_SpanAvailWidth;

		const ImGuiID ActorImGuiID = ImGui::GetID((void*)(uint64_t)(uint32_t)Handle);
		std::string_view Name = Actor->GetName();
		char NodeName[84];
		if (Actor->IsPlayer())
		{
			/* @fixme: Skip UUID for player until spawning and serialization for player is handled. */
			std::snprintf(NodeName, sizeof(NodeName), "%s", Name.data());
		}
		else
		{
			std::snprintf(NodeName, sizeof(NodeName), "%s (%lld)", Name.data(), static_cast<LUUID::SizeType>(Handle));
		}

		const bool WasNodeOpen = ImGui::TreeNodeBehaviorIsOpen(ActorImGuiID);
		const bool NodeOpened = ImGui::TreeNodeEx((void*)ActorImGuiID, TreeNodeFlags, NodeName);
		if (NodeOpened)
		{
			ActorNode_Data(Actor);
			UI::Widget::DrawComponents(Actor);
			UI::Widget::ActorDeleteButton(Actor, Scene);
			ImGui::TreePop();
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
		if (!Actor)
		{
			return;
		}

		UI::Widget::DrawComponent<FTransformComponent>("Transform", Actor, [Actor](const FTransformComponent& TC)
		{
			UI::BeginPropertyGrid();
			bool Changed = false;

			/* Translation */
			ImGui::TableNextRow();
			glm::vec3 Translation = TC.GetTranslation();
			Changed |= UI::Widget::Vec3Control("Translation", Translation, 0.0f, 0.010f, -100.0f, 100.0f);
			if (Changed)
			{
				Actor->SetPosition(Translation);
			}

			/* Rotation */
			ImGui::TableNextRow();
			float Rotation = glm::degrees(TC.GetRotation2D());
			Changed |= UI::Widget::DragFloat("Rotation", Rotation, 0.10f, (-6 * 360.0f), (6 * 360.0f), "%.2f");
			if (Changed)
			{
				Actor->SetRotation(glm::radians(Rotation));
			}

			UI::EndPropertyGrid();
		});

		UI::Widget::DrawComponent<FEffectComponent>("Effect", Actor, [Actor](FEffectComponent& EC)
		{
			UI::BeginPropertyGrid();
			for (auto& Effect : EC.Effects)
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::Text("%s", Enum::ToString(Effect.Type));
				switch (Effect.Type)
				{
					case EEffectType::Rotate:
					{
						ImGui::TableNextRow();
						ImGui::TableSetColumnIndex(1);

						auto& EffectRef = std::get<FRotateEffect>(Effect.Data);
						UI::Widget::DragFloat("Angular Speed (deg/s)", EffectRef.AngularSpeedDegPerSecond, 1.0f, -360.0f, 360.0f);
						break;
					}

					default:
						break;
				}
			}

			ImGui::Dummy(ImVec2(0, 20));
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);

				static std::size_t SelectedIdx = std::to_underlying(EEffectType::Rotate);
				static const std::array<EEffectType, std::to_underlying(EEffectType::COUNT)> EffectTypes = {
					EEffectType::None,
					EEffectType::Rotate,
				};

				static EEffectType EffectType = EEffectType::Rotate;
				const float ComboItemWidth = ((ImGui::GetContentRegionAvail().x - 8.0f) / 2.0f);
				ImGui::SetNextItemWidth(ComboItemWidth);
				if (ImGui::BeginCombo("##EffectType", Enum::ToString(EffectType)))
				{
					for (int Idx = 0; Idx < EffectTypes.size(); Idx++)
					{
						const char* Option = Enum::ToString(EffectTypes[Idx]);
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
					if (SelectedIdx != std::to_underlying(EffectType))
					{
						EffectType = static_cast<EEffectType>(SelectedIdx);
					}
				}

				ImGui::TableSetColumnIndex(1);
				{
					UI::FScopedStyle ButtonRounding(ImGuiStyleVar_FrameRounding, 8.0f);
					if (ImGui::Button("Add effect"))
					{
						FEffectInstance Effect;
						Effect.Type = EEffectType::Rotate;
						FRotateEffect Rotate;
						Rotate.AngularSpeedDegPerSecond = 10.0f;
						Effect.Data = Rotate;
						EC.Effects.push_back(Effect);
					}
				}
			}

			UI::EndPropertyGrid();
		});

		/* Button: Add Component */
		{
			UI::FScopedFont Font(UI::Font::Get(EFont::SourceSansPro, EFontSize::Large));
			const float LineHeight = GImGui->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			static const char* AddButtonLabel = "Add Component";
			ImVec2 AddTextSize = ImGui::CalcTextSize(AddButtonLabel);
			ImVec2 AddButtonSize = ImVec2(AddTextSize.x * 1.30f, LineHeight + 2.0f);

			UI::FScopedStyle ButtonRounding(ImGuiStyleVar_FrameRounding, 8.0f);
			if (ImGui::Button(AddButtonLabel, AddButtonSize))
			{
				ImGui::OpenPopup("AddComponent");
			}
		}

		/* Popup: Add Component */
		if (ImGui::BeginPopup("AddComponent", ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking))
		{
			static constexpr float AddCompPanelWidth = 250.0f;
			if (ImGui::BeginTable("##CompTable", 2, ImGuiTableFlags_SizingStretchSame))
			{
				ImGui::TableSetupColumn("Icon", ImGuiTableColumnFlags_WidthFixed, AddCompPanelWidth * 0.12f);
				ImGui::TableSetupColumn("Components", ImGuiTableColumnFlags_WidthFixed, AddCompPanelWidth * 0.88f);

				UI::Widget::DrawAddComponentButton<FTransformComponent>("Transform", EditorResources.PlusIcon, Actor);
				UI::Widget::DrawAddComponentButton<FEffectComponent>("Effect", EditorResources.PlusIcon, Actor);

				ImGui::EndTable();
			}

			ImGui::EndPopup();
		}
	}

}
