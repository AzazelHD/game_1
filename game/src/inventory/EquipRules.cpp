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

bool EquipRules::mainHandSupportsOffhand(const EquipmentLoadout &loadout)
{
    const Gear *mainHand = loadout.slots[static_cast<std::size_t>(GearSlot::Weapon)];
    return weaponSupportsOffhand(mainHand);
}

bool EquipRules::isOffhandEligibleGear(const Gear &gear, const EquipmentLoadout &loadout)
{
    // Offhand is only enabled while a one-handed non-ranged main-hand is
    // equipped. Two-handed or ranged main-hands disable the slot entirely.
    if (!mainHandSupportsOffhand(loadout))
        return false;

    // Shield is the classic off-hand item: tagged with slot == Offhand and
    // not a weapon.
    if (gear.slot() == GearSlot::Offhand && !gear.isWeapon())
        return true;

    // Dual-wield: any one-handed non-ranged weapon (Sword, Mace, …) may
    // fill the Offhand slot alongside a one-handed non-ranged main-hand.
    if (gear.isWeapon())
        return weaponSupportsOffhand(&gear);

    return false;
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

    // Offhand has special eligibility: accepts Shield OR one-handed non-ranged
    // weapon (dual-wield), but only while a one-handed non-ranged main-hand
    // is already equipped. The dual-wield case covers weapons whose
    // `slot()` is GearSlot::Weapon (e.g. Sword/Mace); the shield case
    // covers gear tagged with slot() == GearSlot::Offhand.
    if (gear.slot() == GearSlot::Offhand)
        return isOffhandEligibleGear(gear, loadout);

    // Dual-wield validation: equipping a weapon (slot == Weapon) into an
    // Offhand slot is allowed only when the current main-hand is a
    // one-handed non-ranged weapon. RosterSystem::equip clears the target
    // slot before calling, so we cannot rely on slot-equality here; we
    // accept the request whenever the main-hand permits an off-hand item
    // and the gear is a valid off-hand candidate. RosterSystem passes
    // `slot` as the destination — we don't have it here, so the candidate
    // filter in UnitDetailWindow is the first gate. The validation below
    // also guards the Weapon-slot case where a Shield/dual-wield weapon
    // already occupies Offhand and the new main-hand must still support it.
    if (gear.isWeapon() && gear.weaponHandedness() == WeaponHandedness::OneHanded && !gear.isRanged())
    {
        // This branch is hit both for normal main-hand equips (slot == Weapon)
        // and dual-wield off-hand equips. Reject the main-hand case when an
        // existing off-hand item is incompatible.
        const Gear *offhand = loadout.slots[static_cast<std::size_t>(GearSlot::Offhand)];
        if (offhand != nullptr)
        {
            if (offhand->isWeapon())
                return weaponSupportsOffhand(&gear);
            // Shield in offhand: new main-hand must still be one-handed non-ranged
            return weaponSupportsOffhand(&gear);
        }
        // No offhand currently: the gear is a valid one-handed non-ranged
        // weapon, the Weapon slot is empty (or this is a dual-wield swap),
        // and main-hand is unchanged from loadout. Allow.
        return true;
    }
    if (gear.isWeapon())
    {
        // Two-handed or ranged weapon: cannot be paired with any off-hand item.
        const Gear *offhand = loadout.slots[static_cast<std::size_t>(GearSlot::Offhand)];
        if (offhand != nullptr)
            return false;
        return true;
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