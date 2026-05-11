#include "inventory.h"

namespace platformer2d {

	CInventory::CInventory(std::string_view InName)
		: Name(InName)
	{
		LK_DEBUG_TAG("Inventory", "Created: {}", Name);
	}

	CInventory::~CInventory()
	{
		LK_ASSERT(!IsFull() && (GetFreeSlots() == Items.size()));
	}

	void CInventory::Tick(const float DeltaTime)
	{
		for (FInventoryItem& Entry : Items) {
			if (!IsSlotValid(Entry)) {
				continue;
			}

			if (Entry.ItemRef->GetItemType() == EItemType::Weapon) {
				std::static_pointer_cast<IWeapon>(Entry.ItemRef)->Tick(DeltaTime);
			}
		}
	}

	void CInventory::Destroy()
	{
		LK_DEBUG_TAG("Inventory", "Release: {} (Occupied {}/{})", Name, (Items.size() - GetFreeSlots()), Items.size());

		for (FInventoryItem& Entry : Items) {
			/* @todo: Serialize every entry */
			if (!IsSlotValid(Entry)) {
				continue;
			}

			LK_DEBUG_TAG("Inventory", "Release: {}", Enum::ToString(Entry.ItemRef->GetItemType()));
			Entry.ItemRef.reset();
		}

		LK_ASSERT(!IsFull() && (GetFreeSlots() == Items.size()));
	}

	bool CInventory::IsEmpty() const
	{
		for (std::size_t Idx = 0; Idx < Items.size(); Idx++) {
			if (Items.at(Idx).ItemRef != nullptr) {
				return false;
			}
		}

		return true;
	}

	bool CInventory::IsFull() const
	{
		for (std::size_t Idx = 0; Idx < Items.size(); Idx++) {
			if (Items.at(Idx).ItemRef == nullptr) {
				return false;
			}
		}

		return true;
	}

	std::size_t CInventory::GetFreeSlots() const
	{
		std::size_t Free = 0;
		for (std::size_t Idx = 0; Idx < Items.size(); Idx++) {
			if (Items.at(Idx).ItemRef == nullptr) {
				Free++;
			}
		}

		return Free;
	}

	std::size_t CInventory::GetUsedSlots() const
	{
		return (MAX_ITEMS - GetFreeSlots());
	}

	bool CInventory::GetNextFreeIdx(std::size_t& Idx) const
	{
		Idx = 0;
		for (; Idx < Items.size(); Idx++) {
			if (Items.at(Idx).ItemRef == nullptr) {
				return true;
			}
		}

		Idx = std::numeric_limits<std::size_t>::max();
		return false;
	}

	void CInventory::SetName(std::string_view InName)
	{
		Name = InName;
	}

	std::vector<std::shared_ptr<IItem>> CInventory::Snapshot() const
	{
		std::vector<std::shared_ptr<IItem>> Out;
		Out.reserve(GetUsedSlots());
		for (const FInventoryItem& Entry : Items) {
			if (IsSlotValid(Entry)) {
				Out.push_back(Entry.ItemRef);
			}
		}

		return Out;
	}

	void CInventory::Restore(const std::vector<std::shared_ptr<IItem>>& InItems)
	{
		LK_DEBUG_TAG("Inventory", "[{}] Restore {} items", Name, InItems.size());
		for (std::size_t Idx = 0; Idx < Items.size(); Idx++) {
			ClearSlot(Idx);
		}

		for (const std::shared_ptr<IItem>& Item : InItems) {
			if (!Item) {
				continue;
			}

			std::size_t FreeIdx = 0;
			if (!GetNextFreeIdx(FreeIdx)) {
				LK_WARN_TAG("Inventory", "[{}] Restore truncated, no free slots", Name);
				break;
			}

			FInventoryItem& Entry = Items.at(FreeIdx);
			Entry.ItemRef = Item;
			Entry.Type = Item->GetItemType();
			Entry.bValid = true;
		}
	}

	bool CInventory::Serialize(YAML::Emitter& Out, const EExtendableSerializer Extendable) const
	{
		LK_UNUSED(Extendable);
		LK_DEBUG_TAG("Inventory", "Serializing: {}", Name);

		Out << YAML::Key << "Inventory";
		Out << YAML::BeginMap;
		Out << YAML::Key << "PlayerIdx" << YAML::Value << PlayerIdx;

		Out << YAML::Key << "Items";
		Out << YAML::Value << YAML::BeginSeq;
		for (std::size_t Idx = 0; Idx < Items.size(); Idx++) {
			Out << YAML::BeginMap;
			Out << YAML::Key << "PlayerIdx" << YAML::Value << PlayerIdx;
			Out << YAML::EndMap;
		}
		Out << YAML::Value << YAML::EndSeq;

		Out << YAML::EndMap; /* ~Inventory */

		return true;
	}

	void CInventory::ClearSlot(const std::size_t Idx)
	{
		LK_ASSERT(Idx < Items.size());
		FInventoryItem& Entry = Items.at(Idx);
		Entry.ItemRef.reset();
		Entry.Type = EItemType::None;
		Entry.bValid = false;
	}
}

