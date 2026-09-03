#include "inventory/GearCatalog.h"

#include "systems/RosterSystem.h"

bool GearCatalog::add(std::unique_ptr<Gear> gear)
{
    if (!gear || !isValidItemDefinition(*gear) || m_gears.count(gear->id) != 0)
        return false;

    m_gears.emplace(gear->id, std::move(gear));
    return true;
}

const Gear *GearCatalog::find(ItemId itemId) const
{
    const auto it = m_gears.find(itemId);
    return it == m_gears.end() ? nullptr : it->second.get();
}

const ItemDefinition *GearCatalog::findItem(ItemId itemId) const
{
    return find(itemId);
}

EquipmentLoadout GearCatalog::resolve(const EquippedGearIds &equipped) const
{
    EquipmentLoadout resolved;
    for (std::size_t index = 0; index < equipped.slots.size(); ++index)
    {
        if (equipped.slots[index].has_value())
            resolved.slots[index] = find(*equipped.slots[index]);
    }
    for (std::size_t index = 0; index < equipped.accessories.size(); ++index)
    {
        if (equipped.accessories[index].has_value())
            resolved.accessories[index] = find(*equipped.accessories[index]);
    }
    return resolved;
}