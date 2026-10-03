#pragma once
#include "battle/unit/SkillProgression.h"
#include "inventory/EquipRules.h"

#include <array>
#include <optional>
#include <string>
#include <vector>

class GearCatalog;
class Inventory;

// Persistent campaign equipment references. Equipped IDs are removed from the
// shared Inventory while equipped (and returned on unequip), so a slot cannot
// be double-held. Identical gear is one stacking inventory entry; each equip
// consumes one unit, still reserving the rest for other units until the stack
// is exhausted.
struct EquippedGearIds
{
    std::array<std::optional<ItemId>, kGearSlotCount> slots{};
    std::array<std::optional<ItemId>, kAccessorySlotCount> accessories{};
};

struct RosterUnit
{
    int instanceId = -1;      // assigned once at recruitment, never reused or changed — the real unique key
    std::string templatePath; // which class/job this is — e.g. "assets/units/soldier.json"
    std::string customName;   // empty = use the template's default name; set for named uniques (Marche, Montblanc) or renamed generics
    bool recruited = true;
    int exp = 0;

    // Source of truth for the unit's level. Battle units are rebuilt fresh
    // from their template each battle and grown to this level via
    // Unit::growToLevel(). Surviving roster units gain +1 per battle victory
    // (see BattleState::applyVictoryRewards), and each level-up also grants
    // +1 skill point (see skillPoints).
    int level = 1;

    // Progression (M4). Level-ups grant skill points, points are spent
    // permanently to unlock skills from the progression table, and unlocked
    // skills can be equipped to the active-skill loadout. The battle unit
    // copies this state on construction (Unit::importRosterProgression), and
    // the campaign save persists it.
    BaseClass baseClass = BaseClass::Soldier;
    int skillPoints = 0;
    SkillProgression skills;

    EquippedGearIds equippedGear;
};

class RosterSystem
{
public:
    void setUnits(std::vector<RosterUnit> units);
    void ensureDefaultUnits();

    // Adds a new unit with a fresh, never-reused instanceId. Returns a
    // pointer into m_units (valid until the next structural change to the
    // vector — same caveat as any vector-of-value API).
    RosterUnit *recruit(std::string templatePath, std::string customName = "");

    // Removes the unit with this instanceId, if present. The vector
    // reorders/shrinks freely — instanceId is never reassigned to anyone
    // else, so nothing else needs to change when this happens.
    void dismiss(int instanceId);

    const std::vector<RosterUnit> &units() const { return m_units; }
    const RosterUnit *findById(int instanceId) const;
    RosterUnit *findById(int instanceId);
    const RosterUnit *findByRef(const std::string &unitRef) const;

    EquipmentLoadout resolveLoadout(const RosterUnit &unit, const GearCatalog &catalog) const;

    // Inventory ownership moves atomically: equipped gear is absent from the
    // shared inventory, and displaced gear is returned in the same operation.
    bool equip(int instanceId, GearSlot slot, int accessoryIndex, ItemId itemId,
               Inventory &inventory, const GearCatalog &catalog, Race race);
    bool unequip(int instanceId, GearSlot slot, int accessoryIndex,
                 Inventory &inventory, const GearCatalog &catalog);

private:
    std::vector<RosterUnit> m_units;
    int m_nextInstanceId = 0; // monotonically increasing, never decremented or reused
};
