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

void Unit::kill()
{
    if (isDead())
        return;
    m_currentHp = 0;
    m_state = UnitState::Dead;
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

    // Tick durations one step at the START of the unit's own turn: a mod set
    // with N turns is active for N of the unit's further actions, then drops
    // off before the (N+1)-th action menu opens.
    for (TemporaryStatMod &mod : m_statMods)
        if (mod.turnsRemaining > 0)
            --mod.turnsRemaining;
    m_statMods.erase(std::remove_if(m_statMods.begin(), m_statMods.end(),
                                    [](const TemporaryStatMod &mod)
                                    { return mod.turnsRemaining <= 0; }),
                     m_statMods.end());
}

// ── Status effects ──────────────────────────────────────────────────────────
void Unit::applyStatMod(StatKind stat, int amount, int turns)
{
    m_statMods.erase(std::remove_if(m_statMods.begin(), m_statMods.end(),
                                    [stat](const TemporaryStatMod &mod)
                                    { return mod.stat == stat; }),
                     m_statMods.end());
    if (amount != 0 && turns > 0)
        m_statMods.push_back(TemporaryStatMod{.stat = stat, .amount = amount, .turnsRemaining = turns});
}

int Unit::activeStatMod(StatKind stat) const
{
    return tempMod(stat);
}

int Unit::tempMod(StatKind stat) const
{
    int sum = 0;
    for (const TemporaryStatMod &mod : m_statMods)
        if (mod.stat == stat)
            sum += mod.amount;
    return sum + statusStatContribution(stat);
}

namespace
{
    // Percentage stat effect baked in per buff/debuff status type. Buffs scale
    // off the BASE stat (data + race/gender bonuses), curses off the TOTAL
    // stat (base + gear) — see Unit::statusStatContribution.
    struct StatusStatEffect
    {
        StatKind stat;
        int pct;       // signed: +25, −10
        bool ofTotal;  // false = % of base stat, true = % of total stat
    };

    constexpr StatusStatEffect kBarrierEffects[] = {
        {StatKind::Defense, 25, false},
        {StatKind::MagicDefense, 25, false},
    };
    constexpr StatusStatEffect kAttackUpEffects[] = {
        {StatKind::Attack, 25, false},
    };
    constexpr StatusStatEffect kArmorDownEffects[] = {
        {StatKind::Defense, -10, true},
        {StatKind::MagicDefense, -10, true},
    };
    constexpr StatusStatEffect kAttackDownEffects[] = {
        {StatKind::Attack, -10, true},
    };

    struct StatusStatEffects
    {
        const StatusStatEffect *effects;
        std::size_t count;
    };

    const StatusStatEffects kStatusEffectsByType[] = {
        {nullptr, 0}, // Poison
        {nullptr, 0}, // Burn
        {nullptr, 0}, // Stun
        {nullptr, 0}, // Sleep
        {kBarrierEffects, 2},   // Barrier
        {kAttackUpEffects, 1},  // AttackUp
        {kArmorDownEffects, 2}, // ArmorDown
        {kAttackDownEffects, 1} // AttackDown
    };
} // namespace

int Unit::statusStatContribution(StatKind stat) const
{
    int total = 0;
    for (const ActiveStatus &status : m_statuses)
    {
        const StatusStatEffects &effects = kStatusEffectsByType[static_cast<std::size_t>(status.type)];
        for (std::size_t i = 0; i < effects.count; ++i)
        {
            const StatusStatEffect &effect = effects.effects[i];
            if (effect.stat != stat)
                continue;

            // Buffs read the base stat; curses read the total (base + gear),
            // deliberately excluding temp/status mods so the basis stays stable.
            int basis;
            switch (stat)
            {
            case StatKind::Attack: basis = effect.ofTotal ? m_data.attack + m_bonuses.attack + gearModifiers().attack
                                                          : m_data.attack; break;
            case StatKind::Defense: basis = effect.ofTotal ? m_data.defense + m_bonuses.defense + gearModifiers().defense
                                                           : m_data.defense; break;
            case StatKind::Magic: basis = effect.ofTotal ? m_data.magic + m_bonuses.magic + gearModifiers().magic
                                                         : m_data.magic; break;
            case StatKind::MagicDefense: basis = effect.ofTotal ? m_data.magicDefense + m_bonuses.magicDef + gearModifiers().magicDefense
                                                                : m_data.magicDefense; break;
            case StatKind::Speed: basis = effect.ofTotal ? m_data.speed + m_bonuses.speed + gearModifiers().speed
                                                         : m_data.speed; break;
            case StatKind::Evasion: basis = effect.ofTotal ? m_data.evasion + m_bonuses.evasion + gearModifiers().evasion
                                                           : m_data.evasion; break;
            default: basis = 0; break;
            }

            const int magnitude = (std::abs(effect.pct) * basis + 50) / 100; // round half up
            total += effect.pct < 0 ? -magnitude : magnitude;
        }
    }
    return total;
}

// ── Battle status effects ────────────────────────────────────────────────────
void Unit::applyStatus(StatusType type, int turns, int damagePerTick)
{
    if (turns <= 0)
        return;

    const StatusInfo &info = statusInfo(type);
    for (ActiveStatus &status : m_statuses)
    {
        if (status.type != type)
            continue;
        if (info.stacks)
            status.damagePerTick += damagePerTick;
        status.turnsRemaining = std::max(status.turnsRemaining, turns);
        return;
    }
    m_statuses.push_back(ActiveStatus{.type = type, .turnsRemaining = turns, .damagePerTick = damagePerTick});
}

void Unit::removeStatus(const StatusType &type)
{
    m_statuses.erase(std::remove_if(m_statuses.begin(), m_statuses.end(),
                                    [&type](const ActiveStatus &status)
                                    { return status.type == type; }),
                     m_statuses.end());
}

bool Unit::hasStatus(StatusType type) const
{
    return std::any_of(m_statuses.begin(), m_statuses.end(),
                       [type](const ActiveStatus &status)
                       { return status.type == type; });
}

int Unit::statusTurns(StatusType type) const
{
    for (const ActiveStatus &status : m_statuses)
        if (status.type == type)
            return status.turnsRemaining;
    return 0;
}

Unit::StatusTickResult Unit::tickStatuses()
{
    StatusTickResult result;
    if (m_statuses.empty())
        return result;

    // 1. Accumulate DoT damage from every status still active this turn, then
    //    resolve turn locks BEFORE counting down — a Stun/Sleep applied with
    //    N turns must consume exactly N of the target's turns.
    for (const ActiveStatus &status : m_statuses)
    {
        if (status.damagePerTick > 0)
        {
            result.dotDamage += status.damagePerTick;
            result.dotType = status.type;
        }
        if (!result.skippedTurn && statusInfo(status.type).skipsTurn)
        {
            result.skippedTurn = true;
            result.skipType = status.type;
        }
    }

    // 2. Deal DoT damage (does not wake sleepers — sleep breaks on real
    //    damage only, see BattleSession::applyDamage).
    if (result.dotDamage > 0)
        takeDamage(result.dotDamage);

    // 3. Count down remaining durations and drop expired statuses.
    for (ActiveStatus &status : m_statuses)
        if (status.turnsRemaining > 0)
            --status.turnsRemaining;
    m_statuses.erase(std::remove_if(m_statuses.begin(), m_statuses.end(),
                                    [](const ActiveStatus &status)
                                    { return status.turnsRemaining <= 0; }),
                     m_statuses.end());

    return result;
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

void Unit::growToLevel(int targetLevel)
{
    int capped = std::max(1, targetLevel);
    if (m_data.level >= capped)
        return;
    while (m_data.level < capped)
    {
        if (!levelUp())
            break;
    }
}

bool Unit::levelUp()
{
    // Max level
    if (m_data.level >= getMaxLevel())
        return false;

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

    return true;
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

void Unit::importRosterProgression(int skillPoints, const SkillProgression &progression)
{
    m_skillPoints = skillPoints;
    m_skillProgression = progression;

    // Equipped Active skills become battle actions. The progression id IS the
    // combat skill id for active skills, so it can be appended straight onto
    // the unit's skill list — the in-battle Skills menu and combat resolution
    // read skillIds + skillDB and need nothing else.
    for (const std::string &skillId : m_skillProgression.equippedActive())
    {
        if (std::find(m_data.skillIds.begin(), m_data.skillIds.end(), skillId) == m_data.skillIds.end())
            m_data.skillIds.push_back(skillId);
    }
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
