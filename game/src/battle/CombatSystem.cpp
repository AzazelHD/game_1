// CombatSystem.cpp
#include "CombatSystem.h"
#include "Unit.h"
#include <algorithm>
#include <cstdlib>

// ---------------------------------------------------------------------------
// public
// ---------------------------------------------------------------------------

CombatResult CombatSystem::resolve(const HitContext &ctx)
{
    CombatResult result;

    if (!rollHit(ctx))
        return result;

    result.hit = true;
    result.crit = rollCrit();

    int offense = computeDamage(ctx);

    // Element affinity
    if (ctx.element != Element::Neutral)
    {
        result.affinityResult = getAffinity(*ctx.target, ctx.element);
        switch (result.affinityResult)
        {
        case Affinity::Weak:
            offense = static_cast<int>(offense * AFFINITY_WEAK);
            break;
        case Affinity::Neutral:
            break;
        case Affinity::Resistant:
            offense = static_cast<int>(offense * AFFINITY_RESIST);
            break;
        case Affinity::Immune:
            offense = 0;
            break;
        case Affinity::Absorb:
            result.absorbed = true;
            break;
        }
    }

    // Crit amplifies offense before defense
    if (result.crit)
        offense = static_cast<int>(offense * CRIT_MULTIPLIER);

    // Subtract defense
    const Unit &def = *ctx.target;
    int defense = ctx.isMagical ? def.getMagicDefense() : def.getDefense();
    int dmg = offense - defense;

    result.damage = std::max(0, dmg);
    return result;
}

// ---------------------------------------------------------------------------
// private
// ---------------------------------------------------------------------------

bool CombatSystem::rollHit(const HitContext &ctx)
{
    const int hitChance = ctx.skillAccuracy - ctx.target->getEvasion() + sideAccuracyBonus(ctx.side) + ctx.tileEvasionBonus;

    const int roll = std::rand() % 100; // 0–99
    return roll < std::clamp(hitChance, 0, 100);
}

bool CombatSystem::rollCrit()
{
    const int roll = std::rand() % 100;
    return roll < static_cast<int>(CRIT_CHANCE * 100.f);
}

int CombatSystem::computeDamage(const HitContext &ctx)
{
    const Unit &atk = *ctx.attacker;
    const Unit &def = *ctx.target;

    // Raw offensive power before defense
    int offense = ctx.basePower + (ctx.isMagical ? atk.getMagic() : atk.getAttack());

    // Small random variance ±10%
    const int variance = static_cast<int>(offense * 0.10f);
    if (variance > 0)
        offense += (std::rand() % (variance * 2 + 1)) - variance;

    // Side modifier
    switch (ctx.side)
    {
    case AttackSide::Front:
        offense = static_cast<int>(offense * FRONT_DMG_MULTIPLIER);
        break;
    case AttackSide::Side:
        break;
    case AttackSide::Rear:
        offense = static_cast<int>(offense * REAR_DMG_MULTIPLIER);
        break;
    }

    return offense;
}

Affinity CombatSystem::getAffinity(const Unit &target, Element element)
{
    for (const auto &ea : target.getAffinities())
        if (ea.element == element)
            return ea.affinity;

    return Affinity::Neutral;
}

int CombatSystem::sideAccuracyBonus(AttackSide side)
{
    switch (side)
    {
    case AttackSide::Front:
        return 0;
    case AttackSide::Side:
        return 10;
    case AttackSide::Rear:
        return 20;
    }
    return 0;
}