#pragma once

#include "inventory/EquipRules.h"

#include <memory>
#include <unordered_map>

// Owns immutable gear definitions for the campaign. Inventory and roster
// equipment store ItemIds; callers resolve those IDs through this catalog.
class GearCatalog
{
public:
    bool add(std::unique_ptr<Gear> gear);

    const Gear *find(ItemId itemId) const;
    const ItemDefinition *findItem(ItemId itemId) const;
    EquipmentLoadout resolve(const struct EquippedGearIds &equipped) const;

private:
    std::unordered_map<ItemId, std::unique_ptr<Gear>> m_gears;
};