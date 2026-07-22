#pragma once
#include <vector>
#include "engine/math/Vec2.h"
#include "battle/UnitData.h"
#include "battle/UnitProgression.h"
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

class Unit
{
public:
    Unit(const UnitData &data, const RaceData &raceData, const GenderData &genderData, Vec2i startPos);

    // Combat
    void takeDamage(int amount);
    void heal(int amount);
    bool isDead() const;

    // Progression
    void gainExp(int amount);
    int expToNextLevel() const;

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
    int getWaitTime() const { return m_waitTime; }

    // Getters (resources)
    int getCurrentHp() const { return m_currentHp; }
    int getCurrentMp() const { return m_currentMp; }
    int getMaxHp() const { return m_data.maxHp + m_bonuses.maxHp; }
    int getMaxMp() const { return m_data.maxMp + m_bonuses.maxMp; }

    // Getters (stats)
    int getSpeed() const { return m_data.speed + m_bonuses.speed; }
    int getAttack() const { return m_data.attack + m_bonuses.attack; }
    int getDefense() const { return m_data.defense + m_bonuses.defense; }
    int getMagic() const { return m_data.magic + m_bonuses.magic; }
    int getMagicDefense() const { return m_data.magicDefense + m_bonuses.magicDef; }
    int getEvasion() const { return m_data.evasion + m_bonuses.evasion; }
    int getJump() const { return m_data.jump; }
    int getMoveRange() const { return m_data.moveRange + m_bonuses.moveRange; }
    int getAtkRange() const { return m_data.atkRange + m_bonuses.atkRange; }

    // Getters (identity — read only, never change at runtime)
    const std::string &getName() const { return m_data.name; }
    Race getRace() const { return m_data.race; }
    Gender getGender() const { return m_data.gender; }
    const std::vector<ElementAffinity> &getAffinities() const { return m_data.affinities; }
    int getTeam() const { return m_data.team; }

    // Getters (actions)
    const std::vector<ActionType> &getActions() const { return m_actions; }
    const std::vector<SkillType> &getSkills() const { return m_skills; }
    const std::vector<std::string> &getSkillIds() const { return m_data.skillIds; }
    const UnitData &getData() const { return m_data; }

    // Setters
    void setPosition(Vec2i pos) { m_position = pos; }
    void setState(UnitState state) { m_state = state; }
    void setWaitTime(int val) { m_waitTime = val; }
    void setCurrentHp(int val) { m_currentHp = std::clamp(val, 0, getMaxHp()); }
    void setCurrentMp(int val) { m_currentMp = std::clamp(val, 0, getMaxMp()); }

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
    int m_waitTime = 0;
    int m_moveRangeLeft = 0; // refilled to getMoveRange() in resetTurn()
    MajorAction m_majorAction = MajorAction::None;
    UnitState m_state = UnitState::Idle;

    // Actions
    std::vector<ActionType> m_actions;
    std::vector<SkillType> m_skills;

    // Internal
    void levelUp();
};
