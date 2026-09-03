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

private:
    static std::size_t slotIndex(GearSlot slot);
    static bool weaponSupportsOffhand(const Gear *weapon);
};