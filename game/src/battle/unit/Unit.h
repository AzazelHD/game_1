#pragma once
#include <vector>
#include <string>
#include "engine/math/Vec2.h"
#include "engine/math/MathUtils.h"
#include "battle/unit/UnitData.h"
#include "battle/unit/UnitProgression.h"
#include "battle/unit/SkillProgression.h"
#include "battle/unit/RecruitmentRules.h"
#include "battle/unit/StatusEffect.h"
#include "inventory/EquipRules.h"
#include <algorithm>

// A unit's turn is built from two independent budgets:
//   - Movement: a points pool (m_moveRangeLeft), spendable across multiple
//     separate move actions in the same turn, as long as points remain and
//     no MajorAction has been taken since the last move (see undo rules
//     below).
//   - MajorAction: at most ONE of Attack / Skill / Defend per turn. Taking
//     any of these locks in all movement performed so far (undo is no
//     longer possible) but does NOT end the turn or block further movement
//     on its own — Attack/Skill specifically allow moving afterward.
//
// Only Wait and Defend end a turn. Defend itself counts as the MajorAction
// AND ends the turn immediately (no movement after Defend). Running out of
// movement points and/or using the MajorAction does NOT auto-end the turn;
// BattleState must still receive an explicit Wait or Defend.
enum class MajorAction
{
    None,
    Attack,
    Skill,
    Defend
};

// Core battle stats that temporary buff/debuff effects can modify.
enum class StatKind
{
    Attack,
    Defense,
    Magic,
    MagicDefense,
    Speed,
    Evasion
};

// A temporary flat stat modifier (buff/debuff) ticking down on the unit's own
// turns. One mod per StatKind — re-applying replaces the active one.
struct TemporaryStatMod
{
    StatKind stat = StatKind::Attack;
    int amount = 0;
    int turnsRemaining = 0;
};

class Unit
{
public:
    Unit(const UnitData &data, const RaceData &raceData, const GenderData &genderData, Vec2i startPos);

    // Combat
    void takeDamage(int amount);
    // Kills the unit outright with no damage roll and no defense check - the
    // consequence of a Ko-effect skill (SkillEffectType::Ko). Idempotent, and
    // it does not go through takeDamage so callers can tell a KO from a big hit.
    void kill();
    void heal(int amount);
    bool isDead() const;

    // Progression
    void gainExp(int amount);
    int expToNextLevel() const;

    // Grows the unit from its current level up to `targetLevel` by applying
    // levelUp() (stat growth) step by step. Used to rebuild a roster unit at
    // its persisted level when a battle unit is constructed from the template.
    // Capped at getMaxLevel(); levels below 1 are ignored.
    void growToLevel(int targetLevel);

    // Turn lifecycle
    // Resets all per-turn state: refills movement points to getMoveRange()
    // and clears the major action lock.
    void resetTurn();

    // Getters (state)
    UnitState getState() const { return m_state; }
    Vec2i getPosition() const { return m_position; }

    // True once the unit has spent at least one movement point this turn.
    // Derived from the points pool rather than stored directly.
    bool hasMoved() const { return m_moveRangeLeft < getMoveRange(); }

    // True once a major action (attack/skill/defend) has been taken this turn.
    bool hasActed() const { return m_majorAction != MajorAction::None; }

    MajorAction getMajorAction() const { return m_majorAction; }

    // Movement points remaining this turn (0..getMoveRange()).
    int getMoveRangeLeft() const { return m_moveRangeLeft; }

    // Getters (progression)
    int getLevel() const { return m_data.level; }
    int getExp() const { return m_exp; }
    int getMaxLevel() const { return m_data.promotion == PromotionClass::None ? 40 : 50; }
    int getSkillPoints() const { return m_skillPoints; }
    int getWaitTime() const { return m_waitTime; }

    // Getters (resources)
    int getCurrentHp() const { return m_currentHp; }
    int getCurrentMp() const { return m_currentMp; }
    int getMaxHp() const { return m_data.maxHp + m_bonuses.maxHp + gearModifiers().maxHp; }
    int getMaxMp() const { return m_data.maxMp + m_bonuses.maxMp + gearModifiers().maxMp; }

    // Getters (stats)
    int getSpeed() const { return m_data.speed + m_bonuses.speed + gearModifiers().speed + tempMod(StatKind::Speed); }
    int getAttack() const { return m_data.attack + m_bonuses.attack + gearModifiers().attack + tempMod(StatKind::Attack); }
    int getDefense() const { return m_data.defense + m_bonuses.defense + gearModifiers().defense + tempMod(StatKind::Defense); }
    int getMagic() const { return m_data.magic + m_bonuses.magic + gearModifiers().magic + tempMod(StatKind::Magic); }
    int getMagicDefense() const { return m_data.magicDefense + m_bonuses.magicDef + gearModifiers().magicDefense + tempMod(StatKind::MagicDefense); }
    int getEvasion() const { return m_data.evasion + m_bonuses.evasion + gearModifiers().evasion + tempMod(StatKind::Evasion); }
    int getJump() const { return m_data.jump + gearModifiers().jump; }
    int getMoveRange() const { return m_data.moveRange + m_bonuses.moveRange + gearModifiers().moveRange; }
    int getAtkRange() const { return m_data.atkRange + m_bonuses.atkRange; }

    // Getters (identity — read only, never change at runtime)
    const std::string &getName() const { return m_data.name; }
    Race getRace() const { return m_data.race; }
    Gender getGender() const { return m_data.gender; }
    BaseClass getBaseClass() const { return m_data.baseClass; }
    PromotionClass getPromotion() const { return m_data.promotion; }
    const std::vector<ElementAffinity> &getAffinities() const { return m_data.affinities; }
    int getTeam() const { return m_data.team; }
    int getRosterInstanceId() const { return m_rosterInstanceId; }

    // Getters (actions)
    const std::vector<ActionType> &getActions() const { return m_actions; }
    const std::vector<SkillType> &getSkills() const { return m_skills; }
    const std::vector<std::string> &getSkillIds() const { return m_data.skillIds; }
    const UnitData &getData() const { return m_data; }
    const SkillProgression &getSkillProgression() const { return m_skillProgression; }
    SkillProgression &getSkillProgression() { return m_skillProgression; }

    // Status/skill state
    // Applies a temporary flat stat modifier (positive = buff, negative =
    // debuff) for `turns` of the target's own turns. Replaces any existing
    // mod on the same StatKind.
    void applyStatMod(StatKind stat, int amount, int turns);
    int activeStatMod(StatKind stat) const;

    // Status effects ---------------------------------------------------------
    // Applies a battle status for `turns` of the target's own turns. DoT
    // statuses (Poison/Burn) stack their damagePerTick on reapplication and
    // refresh the duration; lock statuses (Stun/Sleep) only refresh the
    // duration. See battle/unit/StatusEffect.h for the full semantics.
    void applyStatus(StatusType type, int turns, int damagePerTick);
    // Removes a status immediately (e.g. Sleep breaks on taking damage).
    void removeStatus(const StatusType &type);
    bool hasStatus(StatusType type) const;
    int statusTurns(StatusType type) const;
    const std::vector<ActiveStatus> &getStatuses() const { return m_statuses; }

    // Result of a status tick at the start of a unit's own turn.
    struct StatusTickResult
    {
        int dotDamage = 0;                // DoT damage dealt this tick (0 = none)
        bool skippedTurn = false;         // unit is stunned/asleep -> turn is consumed
        StatusType skipType = StatusType::Stun; // which lock caused the skip
        StatusType dotType = StatusType::Poison; // which status dealt the DoT
    };
    // Called at the START of the unit's own turn (after resetTurn()). Deals
    // the accumulated DoT damage, refreshes durations, removes expired
    // statuses, and reports whether a stun/sleep lock consumes the turn.
    StatusTickResult tickStatuses();

    bool promote(PromotionClass promotion);
    bool learnOrUpgradeSkill(const std::string &skillId);

    // Copies the roster's persistent progression (unspent points + learned /
    // equipped skills) into this battle unit, then grafts its equipped active
    // skills onto the unit's battle skill list so they show up in the in-battle
    // Skills menu. Call after growToLevel() when building a unit from a
    // RosterUnit.
    void importRosterProgression(int skillPoints, const SkillProgression &progression);
    void setSkillPoints(int points) { m_skillPoints = points; }

    void bindEquipmentLoadout(const EquipmentLoadout *loadout);
    void setResolvedEquipmentLoadout(EquipmentLoadout loadout);
    const EquipmentLoadout *equipmentLoadout() const { return m_equipmentLoadout; }
    bool hasGearSpecialEffect(GearSpecialEffect effect) const;

    // Setters
    void setPosition(Vec2i pos) { m_position = pos; }
    // Direction the unit's "front" faces (one of the four iso diagonals).
    // Updated when the unit moves or attacks; used to draw a facing arrow at
    // the unit's feet, and (later) to derive the attack side.
    void setFacing(IsoDirection dir) { m_facing = dir; }
    IsoDirection getFacing() const { return m_facing; }
    void setState(UnitState state) { m_state = state; }
    void setWaitTime(int val) { m_waitTime = val; }
    void setCurrentHp(int val) { m_currentHp = std::clamp(val, 0, getMaxHp()); }
    void setCurrentMp(int val) { m_currentMp = std::clamp(val, 0, getMaxMp()); }
    // Overrides the template unit's display name at build time (used for
    // recruited units with a custom name like Liandra).
    void setDisplayName(std::string name) { m_data.name = std::move(name); }
    void setRosterInstanceId(int id) { m_rosterInstanceId = id; }

    // Spends `cost` movement points (e.g. path cost of a confirmed move).
    // Clamped to [0, getMoveRange()] — callers should validate the move is
    // affordable beforehand (cost <= getMoveRangeLeft()); this is a hard
    // safety clamp, not the primary validation.
    void spendMovePoints(int cost) { m_moveRangeLeft = std::clamp(m_moveRangeLeft - cost, 0, getMoveRange()); }

    // Refunds `amount` movement points (used by move-undo). Clamped to the
    // unit's max move range.
    void refundMovePoints(int amount) { m_moveRangeLeft = std::clamp(m_moveRangeLeft + amount, 0, getMoveRange()); }

    // Marks the major action slot as used. No-op safety: does not validate
    // that it was previously None — callers (BattleState) are responsible
    // for only allowing this when hasActed() == false.
    void setMajorAction(MajorAction action) { m_majorAction = action; }

    // Convenience: spends all remaining movement points at once. Used by AI
    // or any caller that doesn't need granular point-by-point movement and
    // just wants to mark "this unit is done moving" for the turn.
    void exhaustMovement() { m_moveRangeLeft = 0; }

private:
    // Data
    UnitData m_data;
    RaceData m_raceData;
    GenderData m_genderData;
    StatBonuses m_bonuses;

    // Resources
    int m_currentHp = 0;
    int m_currentMp = 0;

    // Progression
    int m_exp = 0;
    int m_skillPoints = 0;
    float m_hpGrowthAcc = 0.f;
    float m_mpGrowthAcc = 0.f;
    float m_attackGrowthAcc = 0.f;
    float m_defenseGrowthAcc = 0.f;
    float m_magicGrowthAcc = 0.f;
    float m_magicDefGrowthAcc = 0.f;
    float m_speedGrowthAcc = 0.f;
    float m_evasionGrowthAcc = 0.f;

    // Turn state
    Vec2i m_position;
    IsoDirection m_facing = IsoDirection::SouthWest;
    int m_waitTime = 0;
    int m_moveRangeLeft = 0; // refilled to getMoveRange() in resetTurn()
    MajorAction m_majorAction = MajorAction::None;
    UnitState m_state = UnitState::Idle;
    int m_rosterInstanceId = -1;

    // Status effects
    std::vector<TemporaryStatMod> m_statMods;
    std::vector<ActiveStatus> m_statuses;

    // Actions
    std::vector<ActionType> m_actions;
    std::vector<SkillType> m_skills;
    SkillProgression m_skillProgression;
    EquipmentLoadout m_ownedEquipmentLoadout{};
    const EquipmentLoadout *m_equipmentLoadout = nullptr; // non-owning external campaign/loadout state

    // Internal
    void applyEquipmentLoadout(const EquipmentLoadout *loadout);
    bool levelUp();
    GearStatModifiers gearModifiers() const;
    int tempMod(StatKind stat) const;
    // Sum of the stat contribution of active buff/debuff statuses (Barrier,
    // AttackUp, ArmorDown, AttackDown). Buffs: +25% of the base stat; curses:
    // −10% of the total stat (data + race/gender + gear). Folded into tempMod()
    // so every stat getter reflects them. See StatusEffect.h.
    int statusStatContribution(StatKind stat) const;
};
