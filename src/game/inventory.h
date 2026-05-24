#pragma once

#include <vector>

#include "core/core.h"
#include "item.h"
#include "rifle.h"
#include "melee.h"
#include "serialization/serializable.h"

namespace platformer2d {

	namespace Internal {
		template<typename T>
		concept ImplementsItemInterface = std::is_base_of_v<IItem, T>;
	}

	class CInventory : public ISerializable<ESerializable::Yaml>
	{
	private:
		struct FInventoryItem
		{
			EItemType Type = EItemType::None;
			std::shared_ptr<IItem> ItemRef = nullptr;
			bool bValid = false;
		};

	public:
		CInventory(std::string_view InName);
		CInventory() = delete;
		CInventory(CInventory&&) = delete;
		CInventory(const CInventory&) = delete;
		~CInventory();

		CInventory& operator=(const CInventory&) = delete;
		CInventory& operator=(CInventory&&) = delete;

		void Tick(float DeltaTime);
		void Destroy();

		template<Internal::ImplementsItemInterface T>
		bool AddItem(std::shared_ptr<T> Item)
		{
			if (!Item) {
				return false;
			}

			std::size_t FreeIdx = 0;
			if (!GetNextFreeIdx(FreeIdx)) {
				return false;
			}

			LK_INFO_TAG("Inventory", "[{}] Add {} to slot {}", Name, Enum::ToString(Item->GetItemType()), FreeIdx);
			FInventoryItem& Entry = Items.at(FreeIdx);
			Entry.ItemRef = Item;
			Entry.Type = Item->GetItemType();
			Entry.bValid = true;

			return true;
		}

		template<Internal::ImplementsItemInterface T>
		[[nodiscard]] std::shared_ptr<T> GetItem(const std::size_t Idx)
		{
			LK_ASSERT(Idx < Items.size());
			const std::shared_ptr<IItem>& Ref = Items.at(Idx).ItemRef;
			if (!Ref) {
				return nullptr;
			}

			return std::static_pointer_cast<T>(Ref);
		}

		template<Internal::ImplementsItemInterface T>
		bool RemoveItem(const std::size_t Idx)
		{
			LK_ASSERT(Idx < Items.size());
			const FInventoryItem& Entry = Items.at(Idx);
			if (!Entry.bValid || !Entry.ItemRef) {
				return false;
			}

			LK_INFO_TAG("Inventory", "[{}] Remove {} from slot {}", Name, Enum::ToString(Entry.Type), Idx);
#ifdef LK_INVENTORY_SANITY_CAST
			std::static_pointer_cast<T>(Entry.ItemRef);
#endif
			ClearSlot(Idx);
			return true;
		}

		template<Internal::ImplementsItemInterface T>
		bool RemoveItem(const std::shared_ptr<T>& Item)
		{
			if (!Item) {
				return false;
			}

			for (std::size_t Idx = 0; Idx < Items.size(); ++Idx) {
				const FInventoryItem& Entry = Items.at(Idx);
				if (!Entry.bValid) {
					continue;
				}

				if (Entry.ItemRef == Item) {
					LK_INFO_TAG("Inventory", "[{}] Remove {} from slot {}", Name, Enum::ToString(Entry.Type), Idx);
					ClearSlot(Idx);
					return true;
				}
			}

			return false;
		}

		template<typename T>
		[[nodiscard]] std::shared_ptr<T> FindFirstOf()
		{
			static_assert(sizeof(T) == 0, "Not specialized for this type");
			return nullptr;
		}

		[[nodiscard]] bool IsEmpty() const;
		[[nodiscard]] bool IsFull() const;
		[[nodiscard]] std::size_t GetFreeSlots() const;
		[[nodiscard]] std::size_t GetUsedSlots() const;
		[[nodiscard]] bool GetNextFreeIdx(std::size_t& Idx) const;
		void SetName(std::string_view InName);

		bool Select(std::size_t Idx);
		[[nodiscard]] std::size_t GetSelectedIdx() const { return SelectedIdx; }
		[[nodiscard]] std::shared_ptr<IWeapon> GetSelectedWeapon() const;

		[[nodiscard]] std::vector<std::shared_ptr<IItem>> Snapshot() const;
		void Restore(const std::vector<std::shared_ptr<IItem>>& Items);

		[[nodiscard]] std::uint8_t GetAssignedPlayerIndex() const { return PlayerIdx; }
		[[nodiscard]] std::string_view GetName() const { return Name; }

		bool Serialize(YAML::Emitter& Out, EExtendableSerializer Extendable = EExtendableSerializer::No) const override;

	private:
		void ClearSlot(std::size_t Idx);
		[[nodiscard]] static bool IsSlotValid(const FInventoryItem& Entry) { return (Entry.ItemRef && Entry.bValid); }

	public:
		static constexpr std::size_t MAX_ITEMS = 6;

	private:
		std::uint8_t PlayerIdx = 0;
		std::string Name;
		std::array<FInventoryItem, MAX_ITEMS> Items;
		std::size_t SelectedIdx = 0;
	};

	template<>
	inline std::shared_ptr<CRifle> CInventory::FindFirstOf()
	{
		for (const auto& Entry : Items) {
			if (!IsSlotValid(Entry)) {
				continue;
			}

			if (Entry.ItemRef->GetItemType() == EItemType::Weapon) {
				std::shared_ptr<IWeapon> Weapon = std::static_pointer_cast<IWeapon>(Entry.ItemRef);
				if (Weapon->GetWeaponType() == EWeaponType::Rifle) {
					return std::static_pointer_cast<CRifle>(Weapon);
				}
			}
		}

		return nullptr;
	}

	template<>
	inline std::shared_ptr<CMelee> CInventory::FindFirstOf()
	{
		for (const auto& Entry : Items) {
			if (!IsSlotValid(Entry)) {
				continue;
			}

			if (Entry.ItemRef->GetItemType() == EItemType::Weapon) {
				std::shared_ptr<IWeapon> Weapon = std::static_pointer_cast<IWeapon>(Entry.ItemRef);
				if (Weapon->GetWeaponType() == EWeaponType::Melee) {
					return std::static_pointer_cast<CMelee>(Weapon);
				}
			}
		}

		return nullptr;
	}

}

