#include "battle/Unit.h"

#include <algorithm>
#include <cmath>

// ── Constructor ──────────────────────────────────────────────────────────────
Unit::Unit(const UnitData &data, const RaceData &raceData, const GenderData &genderData, Vec2i startPos)
    : m_data(data), m_raceData(raceData), m_genderData(genderData), m_position(startPos)
{
    // Apply bonuses once so m_data always represents effective unit stats.
    m_data.maxHp += raceData.bonusMaxHp + genderData.bonusMaxHp;
    m_data.maxMp += raceData.bonusMaxMp + genderData.bonusMaxMp;
    m_data.attack += raceData.bonusAttack + genderData.bonusAttack;
    m_data.defense += raceData.bonusDefense + genderData.bonusDefense;
    m_data.magic += raceData.bonusMagic + genderData.bonusMagic;
    m_data.magicDefense += raceData.bonusMagicDefense + genderData.bonusMagicDefense;
    m_data.moveRange += raceData.bonusMoveRange + genderData.bonusMoveRange;
    m_data.atkRange += raceData.bonusAtkRange + genderData.bonusAtkRange;
    m_data.evasion += raceData.bonusEvasion + genderData.bonusEvasion;
    m_data.speed += raceData.bonusSpeed + genderData.bonusSpeed;

    m_currentHp = m_data.maxHp;
    m_currentMp = m_data.maxMp;

    // TODO: this should be determined by class
    m_actions = {ActionType::Move, ActionType::Attack, ActionType::Item, ActionType::Wait};

    switch (m_data.race)
    {
    case Race::Human:
        m_skills.push_back(SkillType::Foo_Human);
        break;
    case Race::Elf:
        m_skills.push_back(SkillType::Foo_Elf);
        break;
    case Race::Undead:
        m_skills.push_back(SkillType::Foo_Undead);
        break;
    }
}

// ── Combat ───────────────────────────────────────────────────────────────────
void Unit::takeDamage(int amount)
{
    m_currentHp -= amount;
    if (m_currentHp <= 0)
    {
        m_currentHp = 0;
        m_state = UnitState::Dead;
    }
}

void Unit::heal(int amount)
{
    if (isDead())
        return;
    m_currentHp = std::min(getMaxHp(), m_currentHp + amount);
}

bool Unit::isDead() const
{
    return m_state == UnitState::Dead;
}

// ── Turn lifecycle ────────────────────────────────────────────────────────────
void Unit::resetTurn()
{
    m_hasMoved = false;
    m_hasActed = false;
    m_state = UnitState::Idle;
}

void Unit::gainExp(int amount)
{
    if (m_data.level >= 20)
        return;
    m_exp += amount;
    while (m_exp >= expToNextLevel())
    {
        m_exp -= expToNextLevel();
        levelUp();
    }
    if (m_data.level >= 20)
        m_exp = 0;
}

int Unit::expToNextLevel() const
{
    return 100 * m_data.level * m_data.level; // grows quadratically
}

void Unit::levelUp()
{
    // Max level
    if (m_data.level >= 20)
        return;

    m_data.level++;

    const RaceGrowth &growth = getRaceGrowth(m_data.race);

    auto applyGrowth = [](int &stat, float &acc, float perLevel) -> int
    {
        acc += perLevel;
        const int gain = static_cast<int>(std::floor(acc));
        stat += gain;
        acc -= static_cast<float>(gain);

        return gain;
    };

    const int hpGain = applyGrowth(m_data.maxHp, m_hpGrowthAcc, growth.hpPerLevel);
    const int mpGain = applyGrowth(m_data.maxMp, m_mpGrowthAcc, growth.mpPerLevel);
    applyGrowth(m_data.attack, m_attackGrowthAcc, growth.attackPerLevel);
    applyGrowth(m_data.defense, m_defenseGrowthAcc, growth.defensePerLevel);
    applyGrowth(m_data.magic, m_magicGrowthAcc, growth.magicPerLevel);
    applyGrowth(m_data.magicDefense, m_magicDefGrowthAcc, growth.magicDefPerLevel);
    applyGrowth(m_data.speed, m_speedGrowthAcc, growth.speedPerLevel);
    applyGrowth(m_data.evasion, m_evasionGrowthAcc, growth.evasionPerLevel);

    // Keep gains meaningful immediately after leveling while clamping to new max.
    m_currentHp = std::min(m_data.maxHp, m_currentHp + hpGain);
    m_currentMp = std::min(m_data.maxMp, m_currentMp + mpGain);
}