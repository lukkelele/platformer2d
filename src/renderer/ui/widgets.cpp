#include "widgets.h"

#include "core/window.h"
#include "core/input/keyboard.h"
#include "game/instance.h"
#include "core/selectioncontext.h"
#include "game/spawner.h"
#include "renderer/imgui.h"
#include "ui.h"
#include "scene/scene.h"

namespace platformer2d::UI::Draw {

	struct FActorDataEntry
	{
		std::array<char, 64> NameBuf;
	};

	namespace {
		std::unordered_map<LUUID, FActorDataEntry> ActorDataMap;
	}

	void ActorNode_Data(CActor& Actor)
	{
		const LUUID Handle = Actor.GetHandle();
		ImGui::PushID(Handle);

		static constexpr float LabelColumnWidth = 180.0f;
		ImGui::BeginTable("##VectorControl", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoClip);
		ImGui::TableSetupColumn("LabelColumn", 0, LabelColumnWidth);
		ImGui::TableSetupColumn("ValueColumn", ImGuiTableColumnFlags_IndentEnable | ImGuiTableColumnFlags_NoClip, ImGui::GetContentRegionAvail().x - LabelColumnWidth);

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
				std::snprintf(Data.NameBuf.data(), Data.NameBuf.size(), "%s", Actor.GetName().data());
			}
		}

		FTransformComponent& TC = Actor.GetTransformComponent();
		const CBody& Body = Actor.GetBody();

		/* Transform Component */
		ImGui::TableNextRow();
		glm::vec3 Translation = TC.GetTranslation();
		Changed |= UI::Draw::Vec3Control("Translation", Translation, 0.0f, 0.010f, -100.0f, 100.0f);
		if (Changed)
		{
			Actor.SetPosition(Translation);
		}

		/* Rotation */
		ImGui::TableNextRow();
		float Rotation = glm::degrees(TC.GetRotation2D());
		Changed |= UI::Draw::DragFloat("Rotation", &Rotation, 0.10f, (-6 * 360.0f), (6 * 360.0f), "%.2f");
		if (Changed)
		{
			Actor.SetRotation(glm::radians(Rotation));
		}

		/* Scale */
		ImGui::TableNextRow();
		Changed |= UI::Draw::Vec2Control("Scale", TC.Scale, 0.10f, 0.010f, 0.010f);
#if 0
		glm::vec2 Scale = TC.Scale;
		constexpr float LabelColumnWidth = 100.0f;
		Changed |= UI::Draw::Vec2Control("Scale", Scale, 0.10f, 0.010f, 0.010f, 0.0f, LabelColumnWidth);
		if (Changed)
		{
			Actor.GetBody().SetScale(TC.Scale);
		}
#endif

		/* Actor Name */
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
				Actor.SetName(Data.NameBuf.data());
			}
		}

		/* Texture. */
		ImGui::TableNextRow();
		{
			ETexture Texture = Actor.GetTexture();
			if (TextureDropdown(Texture))
			{
				LK_INFO_TAG("UI", "Update {} texture: {}", Actor.GetName(), Enum::ToString(Texture));
				Actor.SetTexture(Texture);
			}
		}

		/* Color. */
		ImGui::TableNextRow();
		{
			const glm::vec4& ColorRef = Actor.GetColor();
			EColor Color = EColor::White;
			const bool ColorDeduced = FColor::DeduceEnum(Color, ColorRef);
			if (!ColorDeduced)
			{
				ImGui::BeginDisabled();
			}
			if (ColorDropdown(Color))
			{
				LK_INFO_TAG("UI", "Update {} color: {}", Actor.GetName(), Enum::ToString(Color));
				Actor.SetColor(FColor::Get(Color));
			}
			if (!ColorDeduced)
			{
				ImGui::EndDisabled();
			}
		}

		/* Tick info. */
		ImGui::TableNextRow();
		{
			Label("Tick");
			NextColumn();
			ImGui::Text("%s", Actor.IsTickEnabled() ? "Enabled" : "Disabled");
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

		/* Components. */
		{
			const bool HasComponents = Actor.HasAnyExcept<FTransformComponent>();
			if (HasComponents)
			{
				ImGui::SetNextItemOpen(true, ImGuiCond_Once);
			}
			else
			{
				ImGui::BeginDisabled();
			}
			if (ImGui::TreeNodeEx("Components"))
			{
				if (FEffectComponent* EC = Actor.TryGetComponent<FEffectComponent>(); EC != nullptr)
				{
					std::size_t Idx = 0;
					for (const auto& Effect : EC->Effects)
					{
						ImGui::Text("Effect %d: %s", ++Idx, Enum::ToString(Effect.Type));
					}
				}

				ImGui::TreePop();
			}

			if (!HasComponents)
			{
				ImGui::EndDisabled();
			}
		}

		ImGui::PopID();
	}

	void ActorNode_Buttons(CActor& Actor, std::shared_ptr<CScene> Scene)
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
				const bool IsDeletable = Actor.IsDeletable();
				if (!IsDeletable)
				{
					ImGui::BeginDisabled();
				}
				if (ImGui::Button("Delete", ButtonSize))
				{
					const LUUID ActorHandle = Actor.GetHandle();
					LK_INFO("Delete: {} ({})", ActorHandle, Actor.GetName());
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
			ActorNode_Data(*Actor);
			ActorNode_Buttons(*Actor, Scene);
			ImGui::TreePop();
		}

		ImGui::PopID();
	}

	void OnActorDeleted(const LUUID ActorHandle)
	{
		LK_DEBUG_TAG("UI", "Removing handle {} from actor cache", ActorHandle);
		ActorDataMap.erase(ActorHandle);
	}

}
