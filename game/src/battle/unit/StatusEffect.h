#pragma once

#include "engine/renderer/Color.h"

#include <string_view>

// Battle status effects. A status is a harm-over-time (Poison/Burn), a turn
// lock (Stun/Sleep), or a flat stat buff/debuff (Barrier/AttackUp and the
// ArmorDown/AttackDown curses). Defined here so the data layer
// (skill JSON -> SkillData), the battle unit, and the UI all share one
// enum + label registry.
//
// Data model:
//   - ActiveStatus is the per-unit runtime instance.
//   - StatusEffectApply is the payload a skill carries ("statuses" array in
//     the skill JSON); AttackResolutionController maps it onto the target.
//
// Semantics (see Unit::applyStatus / Unit::tickStatuses):
//   - durations count the target's OWN turns;
//   - DoT (Poison/Burn) ticks at the start of the target's turn;
//   - Stun/Sleep consume the target's turn while present (Sleep also breaks
//     on taking real damage); both are refreshed by reapplying (max turns);
//   - DoT statuses stack: a re-application adds its damagePerTick and extends
//     the duration; lock statuses do not stack (refresh only);
//   - Buff/debuff statuses (Barrier/AttackUp/ArmorDown/AttackDown) do not
//     stack and simply refresh. Their stat effect is computed on the fly by
//     Unit::tempMod() from the percentage baked into the registry —
//     buffs grant a +25% *base-stat* bonus, curses take −10% of the unit's
//     *total* stat (data + race/gender + gear) — so they expire the moment
//     the status ticks off.
enum class StatusType
{
    Poison,
    Burn,
    Stun,
    Sleep,
    Barrier,
    AttackUp,
    ArmorDown,
    AttackDown,
};

struct StatusInfo
{
    StatusType type;
    const char *label; // "Poison" — combat log / skill preview
    const char *abbr;  // "PSN" — compact panel label
    bool harmful;
    bool stacks;         // reapplication accumulates damagePerTick + refreshes
    bool skipsTurn;      // stun/sleep consume the unit's turn
    bool breaksOnDamage; // sleep breaks when the unit takes real damage
};

inline constexpr StatusInfo kStatusInfos[] = {
    {StatusType::Poison, "Poison", "PSN", true, true, false, false},
    {StatusType::Burn, "Burn", "BRN", true, true, false, false},
    {StatusType::Stun, "Stun", "STN", true, false, true, false},
    {StatusType::Sleep, "Sleep", "SLP", true, false, true, true},
    {StatusType::Barrier, "Barrier", "BRR", false, false, false, false},
    {StatusType::AttackUp, "Attack Up", "ATK+", false, false, false, false},
    {StatusType::ArmorDown, "Armor Down", "ARM-", true, false, false, false},
    {StatusType::AttackDown, "Attack Down", "ATK-", true, false, false, false},
};

inline const StatusInfo &statusInfo(StatusType type)
{
    for (const StatusInfo &info : kStatusInfos)
        if (info.type == type)
            return info;
    return kStatusInfos[0];
}

// DoT tick colors, shared by the battle floating text and the VFX ring so a
// burn tick and a poison tick read as ELEMENTAL DAMAGE — never as the bright
// green a heal popup uses (a "-3" must not look like "+3 HP").
struct StatusTickVisual
{
    Color floatColor; // floating damage numeral
    Color ringColor;  // VFX ring at the ticked unit's feet
};

inline StatusTickVisual statusTickVisual(StatusType type)
{
    switch (type)
    {
    case StatusType::Burn:
        return {Color{255, 135, 55, 255}, Color{255, 120, 40, 255}};
    case StatusType::Poison:
    default:
        return {Color{150, 200, 90, 255}, Color{140, 170, 80, 255}};
    }
}

// Runtime instance of a status currently on a unit.
struct ActiveStatus
{
    StatusType type = StatusType::Poison;
    int turnsRemaining = 0; // in the target's own turns
    int damagePerTick = 0;  // DoT statuses only (Poison/Burn)
};

// A status a skill may inflict (new optional `statuses` array in skill JSON).
struct StatusEffectApply
{
    StatusType type = StatusType::Poison;
    int turns = 0;
    int damagePerTick = 0;
    int chance = 100; // 0–100, rolled per target on a landed hit / supportive cast
};

// Parses a lowercase JSON type string ("poison", "attack_up", ...). Returns
// StatusType::Poison for unknown strings.
inline StatusType statusTypeFromString(const std::string_view &s)
{
    if (s == "burn")
        return StatusType::Burn;
    if (s == "stun")
        return StatusType::Stun;
    if (s == "sleep")
        return StatusType::Sleep;
    if (s == "barrier")
        return StatusType::Barrier;
    if (s == "attack_up" || s == "attackup")
        return StatusType::AttackUp;
    if (s == "armor_down" || s == "armordown")
        return StatusType::ArmorDown;
    if (s == "attack_down" || s == "attackdown")
        return StatusType::AttackDown;
    return StatusType::Poison; // "poison" and unknown
}