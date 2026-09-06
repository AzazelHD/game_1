#include "systems/RosterSystem.h"

#include "inventory/GearCatalog.h"
#include "inventory/Inventory.h"

#include <algorithm>

void RosterSystem::setUnits(std::vector<RosterUnit> units)
{
    m_units = std::move(units);

    // Keep the next-id counter ahead of whatever was loaded (e.g. from a
    // save file), so recruit() never hands out an id that's already in use.
    for (const RosterUnit &u : m_units)
        m_nextInstanceId = std::max(m_nextInstanceId, u.instanceId + 1);
}

void RosterSystem::ensureDefaultUnits()
{
    if (!m_units.empty())
        return;

    recruit("assets/units/aria.json", "Aria");
    recruit("assets/units/soldier.json", "Soldier");
}

RosterUnit *RosterSystem::recruit(std::string templatePath, std::string customName)
{
    m_units.push_back(RosterUnit{
        .instanceId = m_nextInstanceId++,
        .templatePath = std::move(templatePath),
        .customName = std::move(customName),
        .recruited = true,
    });
    return &m_units.back();
}

void RosterSystem::dismiss(int instanceId)
{
    m_units.erase(
        std::remove_if(m_units.begin(), m_units.end(),
                       [instanceId](const RosterUnit &u)
                       { return u.instanceId == instanceId; }),
        m_units.end());
}

const RosterUnit *RosterSystem::findById(int instanceId) const
{
    for (const RosterUnit &u : m_units)
    {
        if (u.instanceId == instanceId)
            return &u;
    }
    return nullptr;
}

RosterUnit *RosterSystem::findById(int instanceId)
{
    for (RosterUnit &u : m_units)
    {
        if (u.instanceId == instanceId)
            return &u;
    }
    return nullptr;
}

const RosterUnit *RosterSystem::findByRef(const std::string &unitRef) const
{
    for (const RosterUnit &u : m_units)
    {
        if (!u.customName.empty() && u.customName == unitRef)
            return &u;
    }
    for (const RosterUnit &u : m_units)
    {
        if (u.templatePath == unitRef)
            return &u;
    }
    return nullptr;
}

EquipmentLoadout RosterSystem::resolveLoadout(const RosterUnit &unit, const GearCatalog &catalog) const
{
    return catalog.resolve(unit.equippedGear);
}

bool RosterSystem::equip(int instanceId, GearSlot slot, int accessoryIndex, ItemId itemId,
                         Inventory &inventory, const GearCatalog &catalog, Race race)
{
    RosterUnit *unit = findById(instanceId);
    const Gear *gear = catalog.find(itemId);
    if (!unit || !gear)
        return false;

    std::optional<ItemId> *destination = nullptr;
    if (slot == GearSlot::Accessory)
    {
        if (accessoryIndex < 0 || accessoryIndex >= static_cast<int>(unit->equippedGear.accessories.size()))
            return false;
        destination = &unit->equippedGear.accessories[static_cast<std::size_t>(accessoryIndex)];
    }
    else
    {
        const std::size_t index = static_cast<std::size_t>(slot);
        if (index >= unit->equippedGear.slots.size())
            return false;
        destination = &unit->equippedGear.slots[index];
    }

    if (destination->has_value() && **destination == itemId)
        return true;
    if (!inventory.has(itemId, 1))
        return false;

    EquipmentLoadout loadout = catalog.resolve(unit->equippedGear);
    if (slot == GearSlot::Accessory)
        loadout.accessories[static_cast<std::size_t>(accessoryIndex)] = nullptr;
    else
        loadout.slots[static_cast<std::size_t>(slot)] = nullptr;

    // Destination-slot-aware validation. The Offhand slot admits Shields
    // (slot == Offhand) and one-handed non-ranged weapons (dual-wield,
    // slot == Weapon), so it cannot be validated by slot-equality — reuse
    // the same eligibility rule the ItemSelect candidate filter applies.
    const bool allowed = (slot == GearSlot::Accessory)
                             ? (gear->slot() == GearSlot::Accessory)
                             : (slot == GearSlot::Offhand)
                                   ? EquipRules::isOffhandEligibleGear(*gear, loadout)
                                   : EquipRules::canEquip(race, *gear, loadout);
    if (!allowed)
        return false;

    const std::optional<ItemId> displaced = *destination;
    if (!inventory.remove(itemId, 1))
        return false;

    if (displaced.has_value())
    {
        const ItemDefinition *oldItem = catalog.findItem(*displaced);
        if (!oldItem || !inventory.add(*oldItem, 1))
        {
            const ItemDefinition *newItem = catalog.findItem(itemId);
            if (newItem)
                (void)inventory.add(*newItem, 1);
            return false;
        }
    }

    *destination = itemId;
    return true;
}

bool RosterSystem::unequip(int instanceId, GearSlot slot, int accessoryIndex,
                           Inventory &inventory, const GearCatalog &catalog)
{
    RosterUnit *unit = findById(instanceId);
    if (!unit)
        return false;

    std::optional<ItemId> *source = nullptr;
    if (slot == GearSlot::Accessory)
    {
        if (accessoryIndex < 0 || accessoryIndex >= static_cast<int>(unit->equippedGear.accessories.size()))
            return false;
        source = &unit->equippedGear.accessories[static_cast<std::size_t>(accessoryIndex)];
    }
    else
    {
        const std::size_t index = static_cast<std::size_t>(slot);
        if (index >= unit->equippedGear.slots.size())
            return false;
        source = &unit->equippedGear.slots[index];
    }

    if (!source->has_value())
        return false;
    const ItemDefinition *item = catalog.findItem(**source);
    if (!item || !inventory.add(*item, 1))
        return false;

    source->reset();
    return true;
}
