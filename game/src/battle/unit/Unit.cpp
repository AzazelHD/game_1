#include "battle/unit/Unit.h"
#include <algorithm>
#include <cmath>

// ── Constructor ──────────────────────────────────────────────────────────────
Unit::Unit(const UnitData &data, const RaceData &raceData, const GenderData &genderData, Vec2i startPos)
    : m_data(data), m_raceData(raceData), m_genderData(genderData),
      m_position(startPos), m_skillProgression(data.baseClass, data.promotion)
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
    case Race::Elin:
        m_skills.push_back(SkillType::Foo_Elin);
        break;
    case Race::Undead:
        m_skills.push_back(SkillType::Foo_Undead);
        break;
    }

    // Movement pool starts full; resetTurn() also does this at the start of
    // every subsequent turn, but the unit needs a valid value before its
    // first turn too (e.g. for UI that reads getMoveRangeLeft() pre-battle).
    m_moveRangeLeft = getMoveRange();
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

// ── Turn lifecycle ──────────────────────────────────────────────────────────
void Unit::resetTurn()
{
    m_moveRangeLeft = getMoveRange();
    m_majorAction = MajorAction::None;
    m_state = UnitState::Idle;
}

void Unit::gainExp(int amount)
{
    if (m_data.level >= getMaxLevel())
        return;
    m_exp += amount;
    while (m_exp >= expToNextLevel())
    {
        m_exp -= expToNextLevel();
        levelUp();
    }
    if (m_data.level >= getMaxLevel())
        m_exp = 0;
}

int Unit::expToNextLevel() const
{
    return 100 * m_data.level * m_data.level; // grows quadratically
}

void Unit::levelUp()
{
    // Max level
    if (m_data.level >= getMaxLevel())
        return;

    m_data.level++;
    ++m_skillPoints;

    const RaceGrowth &growth = getRaceGrowth(m_data.race);
    const ClassGrowth &classGrowth = getClassGrowth(m_data.baseClass, m_data.promotion);

    auto applyGrowth = [](int &stat, float &acc, float perLevel) -> int
    {
        acc += perLevel;
        const int gain = static_cast<int>(std::floor(acc));
        stat += gain;
        acc -= static_cast<float>(gain);

        return gain;
    };

    const int hpGain = applyGrowth(m_data.maxHp, m_hpGrowthAcc, growth.hpPerLevel + classGrowth.hpPerLevel);
    const int mpGain = applyGrowth(m_data.maxMp, m_mpGrowthAcc, growth.mpPerLevel + classGrowth.mpPerLevel);
    applyGrowth(m_data.attack, m_attackGrowthAcc, growth.attackPerLevel + classGrowth.attackPerLevel);
    applyGrowth(m_data.defense, m_defenseGrowthAcc, growth.defensePerLevel + classGrowth.defensePerLevel);
    applyGrowth(m_data.magic, m_magicGrowthAcc, growth.magicPerLevel + classGrowth.magicPerLevel);
    applyGrowth(m_data.magicDefense, m_magicDefGrowthAcc, growth.magicDefPerLevel + classGrowth.magicDefPerLevel);
    applyGrowth(m_data.speed, m_speedGrowthAcc, growth.speedPerLevel + classGrowth.speedPerLevel);
    applyGrowth(m_data.evasion, m_evasionGrowthAcc, growth.evasionPerLevel + classGrowth.evasionPerLevel);

    // Keep gains meaningful immediately after leveling while clamping to new max.
    m_currentHp = std::min(m_data.maxHp, m_currentHp + hpGain);
    m_currentMp = std::min(m_data.maxMp, m_currentMp + mpGain);
}

bool Unit::promote(PromotionClass promotion)
{
    if (m_data.promotion != PromotionClass::None || !canPromote(m_data.baseClass, promotion))
        return false;

    m_data.promotion = promotion;
    m_skillProgression.setClass(m_data.baseClass, promotion);
    return true;
}

bool Unit::learnOrUpgradeSkill(const std::string &skillId)
{
    return m_skillProgression.learnOrUpgrade(skillId, m_skillPoints);
}

GearStatModifiers Unit::gearModifiers() const
{
    GearStatModifiers total;
    if (!m_equipmentLoadout)
        return total;

    const auto add = [&total](const Gear *gear)
    {
        if (!gear)
            return;
        const GearStatModifiers &modifiers = gear->statModifiers();
        total.maxHp += modifiers.maxHp;
        total.maxMp += modifiers.maxMp;
        total.attack += modifiers.attack;
        total.defense += modifiers.defense;
        total.magic += modifiers.magic;
        total.magicDefense += modifiers.magicDefense;
        total.evasion += modifiers.evasion;
        total.speed += modifiers.speed;
        total.moveRange += modifiers.moveRange;
        total.jump += modifiers.jump;
    };

    for (const Gear *gear : m_equipmentLoadout->slots)
        add(gear);
    for (const Gear *gear : m_equipmentLoadout->accessories)
        add(gear);
    return total;
}

bool Unit::hasGearSpecialEffect(GearSpecialEffect effect) const
{
    return m_equipmentLoadout && EquipRules::hasSpecialEffect(*m_equipmentLoadout, effect);
}

void Unit::applyEquipmentLoadout(const EquipmentLoadout *loadout)
{
    const int oldMaxHp = getMaxHp();
    const int oldMaxMp = getMaxMp();

    m_equipmentLoadout = loadout;

    // Gear modifiers change max HP/MP. Ride the delta over to the current
    // value so equipping a +5 Max HP helmet also heals 5 current HP (and
    // unequipping it takes the 5 back), keeping the "wound" constant. Clamp
    // so a max-lowering swap never leaves current above the new max (or
    // below zero).
    const int newMaxHp = getMaxHp();
    const int newMaxMp = getMaxMp();
    m_currentHp = std::clamp(m_currentHp + (newMaxHp - oldMaxHp), 0, newMaxHp);
    m_currentMp = std::clamp(m_currentMp + (newMaxMp - oldMaxMp), 0, newMaxMp);
}

void Unit::bindEquipmentLoadout(const EquipmentLoadout *loadout)
{
    applyEquipmentLoadout(loadout);
}

void Unit::setResolvedEquipmentLoadout(EquipmentLoadout loadout)
{
    m_ownedEquipmentLoadout = std::move(loadout);
    applyEquipmentLoadout(&m_ownedEquipmentLoadout);
}
