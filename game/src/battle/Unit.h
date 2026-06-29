#pragma once
#include <vector>
#include "battle/UnitData.h"
#include "battle/UnitProgression.h"

#include <algorithm>

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
    void resetTurn();

    // Getters (state)
    UnitState getState() const { return m_state; }
    Vec2i getPosition() const { return m_position; }
    bool hasMoved() const { return m_hasMoved; }
    bool hasActed() const { return m_hasActed; }

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

    // Setters
    void setPosition(Vec2i pos) { m_position = pos; }
    void setState(UnitState state) { m_state = state; }
    void setHasMoved(bool val) { m_hasMoved = val; }
    void setHasActed(bool val) { m_hasActed = val; }
    void setWaitTime(int val) { m_waitTime = val; }
    void setCurrentHp(int val) { m_currentHp = std::clamp(val, 0, getMaxHp()); }
    void setCurrentMp(int val) { m_currentMp = std::clamp(val, 0, getMaxMp()); }

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
    bool m_hasMoved = false;
    bool m_hasActed = false;
    UnitState m_state = UnitState::Idle;

    // Actions
    std::vector<ActionType> m_actions;
    std::vector<SkillType> m_skills;

    // Internal
    void levelUp();
};