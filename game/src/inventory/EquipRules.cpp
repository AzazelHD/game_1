#include "inventory/EquipRules.h"

#include <algorithm>

namespace
{
    constexpr std::array<GearSlot, kGearSlotCount> kSlots = {
        GearSlot::Weapon,
        GearSlot::Offhand,
        GearSlot::Head,
        GearSlot::Body,
        GearSlot::Accessory,
    };

    RaceEquipConfig makeDisabledConfig(Race race)
    {
        RaceEquipConfig config;
        config.race = race;
        for (std::size_t index = 0; index < kGearSlotCount; ++index)
            config.slots[index] = SlotConfig{.slot = kSlots[index], .capacity = 0, .enabled = false};
        return config;
    }

    RaceEquipConfig makeHumanConfig()
    {
        RaceEquipConfig config = makeDisabledConfig(Race::Human);
        for (std::size_t index = 0; index < kGearSlotCount; ++index)
            config.slots[index] = SlotConfig{.slot = kSlots[index], .capacity = 1, .enabled = true};
        return config;
    }

    bool gearHasSpecialEffect(const Gear *gear, GearSpecialEffect effect)
    {
        return gear && gear->specialEffect() == effect;
    }
}

const RaceEquipConfig &EquipRules::configFor(Race race)
{
    static const RaceEquipConfig human = makeHumanConfig();
    // Elf and Elin intentionally use the Human shape until a future race
    // design defines equipment-slot differences.
    static const RaceEquipConfig elf = []
    {
        RaceEquipConfig config = makeHumanConfig();
        config.race = Race::Elf;
        return config;
    }();
    static const RaceEquipConfig elin = []
    {
        RaceEquipConfig config = makeHumanConfig();
        config.race = Race::Elin;
        return config;
    }();
    static const RaceEquipConfig undead = makeDisabledConfig(Race::Undead);

    switch (race)
    {
    case Race::Human:
        return human;
    case Race::Elf:
        return elf;
    case Race::Elin:
        return elin;
    case Race::Undead:
        return undead;
    }
    return undead;
}

std::size_t EquipRules::slotIndex(GearSlot slot)
{
    for (std::size_t index = 0; index < kGearSlotCount; ++index)
        if (kSlots[index] == slot)
            return index;
    return kGearSlotCount;
}

bool EquipRules::isSlotEnabled(Race race, GearSlot slot)
{
    const std::size_t index = slotIndex(slot);
    return index < kGearSlotCount && configFor(race).slots[index].enabled;
}

bool EquipRules::weaponSupportsOffhand(const Gear *weapon)
{
    // Offhand accepts either a Shield or a one-handed, non-ranged weapon (dual-wield).
    // This is a temporary placeholder; later may add class/skill gating.
    return weapon &&
           weapon->weaponHandedness() == WeaponHandedness::OneHanded &&
           !weapon->isRanged();
}

bool EquipRules::canEquip(Race race, const Gear &gear, const EquipmentLoadout &loadout)
{
    if (!isSlotEnabled(race, gear.slot()))
        return false;

    const std::size_t index = slotIndex(gear.slot());
    if (index >= kGearSlotCount)
        return false;

    // Accessory slots can hold multiple items (up to 2)
    if (gear.slot() == GearSlot::Accessory)
    {
        const std::size_t occupied = static_cast<std::size_t>(std::count_if(
            loadout.accessories.begin(), loadout.accessories.end(),
            [](const Gear *item)
            { return item != nullptr; }));
        return occupied < kAccessorySlotCount;
    }

    // Regular slots can only hold one item
    if (loadout.slots[index] != nullptr)
        return false;

    // Offhand has special eligibility: accepts Shield OR one-handed non-ranged weapon (dual-wield)
    if (gear.slot() == GearSlot::Offhand)
    {
        if (gear.slot() == GearSlot::Offhand && !gear.isWeapon())
        {
            // Shield is allowed
            return true;
        }
        // Weapon in Offhand: check main-hand compatibility
        return gear.isWeapon() && weaponSupportsOffhand(&gear);
    }

    // If changing the main-hand weapon, ensure any equipped Offhand remains valid
    if (gear.slot() == GearSlot::Weapon && loadout.slots[slotIndex(GearSlot::Offhand)] != nullptr)
    {
        const Gear *offhand = loadout.slots[slotIndex(GearSlot::Offhand)];
        if (offhand && offhand->isWeapon())
        {
            // Weapon in Offhand: new main-hand must support it
            return weaponSupportsOffhand(&gear);
        }
        // Shield in Offhand: new main-hand must support it
        return weaponSupportsOffhand(&gear);
    }

    return true;
}

bool EquipRules::hasSpecialEffect(const EquipmentLoadout &loadout, GearSpecialEffect effect)
{
    if (effect == GearSpecialEffect::None)
        return false;

    for (const Gear *gear : loadout.slots)
        if (gearHasSpecialEffect(gear, effect))
            return true;
    for (const Gear *gear : loadout.accessories)
        if (gearHasSpecialEffect(gear, effect))
            return true;
    return false;
}