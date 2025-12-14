#pragma once

#include "core/core.h"
#include "serialization/serializable.h"
#include "item.h"

namespace platformer2d {

	/* @todo: Move to its own file */
	class CConsumable : public IItem
	{
	public:
		CConsumable() = default;
		~CConsumable() = default;

		virtual EItemType GetItemType() const override { return EItemType::Consumable; }
	};

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
		~CInventory();

		void Destroy();

		template<typename T>
		bool AddItem(std::shared_ptr<T> Item)
		{
			static_assert(std::is_base_of_v<IItem, T>, "T must implement the IItem interface");
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

		template<typename T>
		std::shared_ptr<T> GetItem(const std::size_t Idx)
		{
			static_assert(std::is_base_of_v<IItem, T>, "T must implement the IItem interface");
			LK_ASSERT(Idx < Items.size());
			const std::shared_ptr<IItem>& Ref = Items.at(Idx).ItemRef;
			if (!Ref) {
				return nullptr;
			}

			return std::static_pointer_cast<T>(Ref);
		}

		template<typename T>
		bool RemoveItem(const std::size_t Idx)
		{
			static_assert(std::is_base_of_v<IItem, T>, "T must implement the IItem interface");
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

		template<typename T>
		bool RemoveItem(const std::shared_ptr<T>& Item)
		{
			static_assert(std::is_base_of_v<IItem, T>, "T must implement the IItem interface");
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

		bool IsFull() const;
		std::size_t GetFreeSlots() const;
		bool GetNextFreeIdx(std::size_t& Idx) const;

		uint8_t GetAssignedPlayerIndex() const { return PlayerIdx; }
		std::string_view GetName() const { return Name; }

		virtual bool Serialize(YAML::Emitter& Out, EExtendableSerializer Extendable = EExtendableSerializer::No) const override;

	private:
		void ClearSlot(std::size_t Idx);

		CInventory(const CInventory&) = delete;
		CInventory(CInventory&&) = delete;
		CInventory& operator=(const CInventory&) = delete;
		CInventory& operator=(CInventory&&) = delete;

	public:
		static constexpr std::size_t MAX_ITEMS = 6;
	private:
		uint8_t PlayerIdx = 0;
		std::string Name;
		std::array<FInventoryItem, MAX_ITEMS> Items;
	};

}
