// CombatSystem.cpp
#include "battle/Unit.h"
#include "battle/CombatSystem.h"

#include <random>
#include <algorithm>

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

    int offense = computeDamage(ctx); // base offense + variance + side multiplier

    applyAffinity(ctx, offense, result);

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

CombatResult CombatSystem::preview(const HitContext &ctx)
{
    CombatResult res;

    // Calculate hit chance
    int hitChance = ctx.skillAccuracy - ctx.target->getEvasion() + sideAccuracyBonus(ctx.side) + ctx.tileEvasionBonus;
    hitChance = std::clamp(hitChance, 0, 100);
    res.hitChance = static_cast<float>(hitChance);

    // Raw offense (no variance, no crit) + side multiplier
    int offense = computeBaseOffense(ctx);

    applyAffinity(ctx, offense, res);

    // Subtract defense
    const Unit &def = *ctx.target;
    int defense = ctx.isMagical ? def.getMagicDefense() : def.getDefense();
    res.damage = std::max(0, offense - defense);

    return res;
}

// ---------------------------------------------------------------------------
// private
// ---------------------------------------------------------------------------

bool CombatSystem::rollHit(const HitContext &ctx)
{
    const int hitChance = ctx.skillAccuracy - ctx.target->getEvasion() + sideAccuracyBonus(ctx.side) + ctx.tileEvasionBonus;

    std::uniform_int_distribution<int> dist(0, 99);
    const int roll = dist(rng());
    return roll < std::clamp(hitChance, 0, 100);
}

bool CombatSystem::rollCrit()
{
    std::uniform_int_distribution<int> dist(0, 99);
    const int roll = dist(rng());
    return roll < static_cast<int>(CRIT_CHANCE * 100.f);
}

int CombatSystem::computeDamage(const HitContext &ctx)
{
    const Unit &atk = *ctx.attacker;

    // Raw offensive power before variance/side (matches preview()'s starting point)
    int offense = ctx.basePower + (ctx.isMagical ? atk.getMagic() : atk.getAttack());

    // Small random variance ±10% — applied BEFORE the side multiplier, unlike
    // preview() which has no variance at all. This is why computeDamage()
    // can't just call computeBaseOffense() directly: the variance step has
    // to be inserted between the raw stat total and the side multiplier.
    const int variance = static_cast<int>(offense * 0.10f);
    if (variance > 0)
        offense += rollVariance(variance);

    return applySideMultiplier(offense, ctx.side);
}

int CombatSystem::computeBaseOffense(const HitContext &ctx)
{
    const Unit &atk = *ctx.attacker;
    int offense = ctx.basePower + (ctx.isMagical ? atk.getMagic() : atk.getAttack());
    return applySideMultiplier(offense, ctx.side);
}

Affinity CombatSystem::getAffinity(const Unit &target, Element element)
{
    for (const auto &ea : target.getAffinities())
        if (ea.element == element)
            return ea.affinity;

    return Affinity::Neutral;
}

void CombatSystem::applyAffinity(const HitContext &ctx, int &offense, CombatResult &outResult)
{
    if (ctx.element == Element::Neutral)
        return;

    outResult.affinityResult = getAffinity(*ctx.target, ctx.element);
    switch (outResult.affinityResult)
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
        outResult.absorbed = true;
        break;
    }
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

int CombatSystem::applySideMultiplier(int offense, AttackSide side)
{
    switch (side)
    {
    case AttackSide::Front:
        return static_cast<int>(offense * FRONT_DMG_MULTIPLIER);
    case AttackSide::Side:
        return offense;
    case AttackSide::Rear:
        return static_cast<int>(offense * REAR_DMG_MULTIPLIER);
    }
    return offense;
}

std::mt19937 &CombatSystem::rng()
{
    // Function-local static: seeded once, on first use, with a real random
    // source — not std::rand()'s process-global, unseeded state (which is
    // also shared with anything else in the codebase that happens to call
    // std::rand()). Not thread-safe if combat ever resolves from multiple
    // threads, but neither was std::rand() (it's global mutable state too).
    static std::mt19937 generator{std::random_device{}()};
    return generator;
}

int CombatSystem::rollVariance(int variance)
{
    std::uniform_int_distribution<int> dist(-variance, variance);
    return dist(rng());
}
