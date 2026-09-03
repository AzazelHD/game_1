#pragma once

#include "battle/unit/UnitData.h"
#include "inventory/Gear.h"

#include <array>
#include <cstddef>
#include <cstdint>

inline constexpr std::size_t kGearSlotCount = 5;
inline constexpr std::size_t kAccessorySlotCount = 2;

struct SlotConfig
{
    GearSlot slot = GearSlot::Weapon;
    std::uint8_t capacity = 0;
    bool enabled = false;
};

struct RaceEquipConfig
{
    Race race = Race::Human;
    std::array<SlotConfig, kGearSlotCount> slots{};
};

// A read-only view of current gear. Equipping and unequipping remain outside
// this subsystem; this structure only supplies state to validation calls.
struct EquipmentLoadout
{
    std::array<const Gear *, kGearSlotCount> slots{};
    std::array<const Gear *, kAccessorySlotCount> accessories{};
};

class EquipRules
{
public:
    static const RaceEquipConfig &configFor(Race race);
    static bool isSlotEnabled(Race race, GearSlot slot);
    static bool canEquip(Race race, const Gear &gear, const EquipmentLoadout &loadout);
    static bool hasSpecialEffect(const EquipmentLoadout &loadout, GearSpecialEffect effect);

    // True if `gear` is a valid candidate for the Offhand slot given the
    // current main-hand state in `loadout`. Accepts both Shields (slot ==
    // Offhand) and one-handed non-ranged weapons (dual-wield). Used both
    // by canEquip()'s validation and by the candidate-filter that builds
    // the ItemSelect list — keeping both in sync here prevents the bug
    // where the validation accepts a candidate the filter rejects (or vice
    // versa).
    static bool isOffhandEligibleGear(const Gear &gear, const EquipmentLoadout &loadout);

private:
    static std::size_t slotIndex(GearSlot slot);
    static bool weaponSupportsOffhand(const Gear *weapon);
    // True if the main-hand weapon in `loadout` permits a secondary item
    // in the Offhand slot (Shield or dual-wield weapon). The offhand slot
    // is only enabled while a one-handed non-ranged main-hand is equipped.
    static bool mainHandSupportsOffhand(const EquipmentLoadout &loadout);
};