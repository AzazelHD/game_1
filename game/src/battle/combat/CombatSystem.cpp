// CombatSystem.cpp
#include "battle/unit/Unit.h"
#include "battle/combat/CombatSystem.h"
#include "battle/combat/SkillEffects.h"
#include "battle/map/BattleMap.h"
#include "data/SkillLoader.h"
#include "engine/data/TileClass.h"

#include <random>
#include <algorithm>

namespace
{
    // Tile-space unit vector the given facing faces toward, matching the
    // engine convention (iso screen diagonals):
    //   NorthEast faces row-1 (up-right), SouthEast col+1 (down-right),
    //   SouthWest row+1 (down-left),  NorthWest col-1 (up-left).
    Vec2i facingTileDelta(IsoDirection facing)
    {
        switch (facing)
        {
        case IsoDirection::NorthEast: return Vec2i{0, -1};
        case IsoDirection::SouthEast: return Vec2i{1, 0};
        case IsoDirection::SouthWest: return Vec2i{0, 1};
        case IsoDirection::NorthWest: return Vec2i{-1, 0};
        }
        return Vec2i{0, 0};
    }
}

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
    hitChance = ctx.neverMiss ? 100 : std::clamp(hitChance, 0, 100);
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

HitContext CombatSystem::hitContextFor(const SkillData &skill, const Unit &attacker, const Unit &target)
{
    HitContext ctx;
    ctx.attacker = &attacker;
    ctx.target = &target;
    ctx.basePower = skill.basePower;
    ctx.isMagical = skill.isMagical;
    ctx.element = skill.element;
    ctx.skillAccuracy = skill.skillAccuracy;
    ctx.neverMiss = skill.neverMiss;
    // Skill-level power scaling (M5): basePower grows by the skill's
    // `powerPerLevel` for each learned level above 1. Units without progression
    // (JSON enemies) report level 0, so they always fight at the authored
    // power. Shared by preview, AI and real hits.
    const int skillLevel = attacker.getSkillProgression().levelOf(skill.id);
    if (skillLevel > 1 && skill.powerPerLevel != 0)
        ctx.basePower += skill.powerPerLevel * (skillLevel - 1);
    // Offensive skills profit from the defender's facing (rear = bonus,
    // front = penalty). Supportive spells on allies stay at the neutral
    // "side" rating — companions don't get flanked by a heal.
    if (skill_effects::isSupportive(skill))
        ctx.side = AttackSide::Side;
    else
        ctx.side = attackSideFrom(attacker, target);
    return ctx;
}

HitContext CombatSystem::basicAttackContext(const Unit &attacker, const Unit &target)
{
    HitContext ctx = hitContextFor(SkillData::makeBasicAttack(), attacker, target);
    ctx.tileEvasionBonus = 0;
    return ctx;
}

void CombatSystem::applyTileBonuses(HitContext &ctx, const BattleMap &map)
{
    if (!ctx.attacker || !ctx.target)
        return;

    const Vec2i atkPos = ctx.attacker->getPosition();
    if (map.isValid(atkPos.x, atkPos.y))
    {
        const uint32_t atkFlags = map.at(atkPos.x, atkPos.y).classFlags;
        if (atkFlags & TileClass::FlagGrassAtk)
            ctx.tileAttackBonus += TILE_ATK_BONUS;
        if (atkFlags & TileClass::FlagGrassMag)
            ctx.tileMagicBonus += TILE_MAG_BONUS;
    }

    const Vec2i defPos = ctx.target->getPosition();
    if (map.isValid(defPos.x, defPos.y))
    {
        const uint32_t defFlags = map.at(defPos.x, defPos.y).classFlags;
        if (defFlags & TileClass::FlagGrassEva)
            ctx.tileEvasionBonus += TILE_EVA_BONUS;
    }
}

bool CombatSystem::chanceRoll(int percent)
{
    if (percent >= 100)
        return true;
    if (percent <= 0)
        return false;
    std::uniform_int_distribution<int> dist(0, 99);
    return dist(rng()) < percent;
}

AttackSide CombatSystem::attackSideFrom(const Unit &attacker, const Unit &defender)
{
    const Vec2i delta = attacker.getPosition() - defender.getPosition();
    if (delta.x == 0 && delta.y == 0)
        return AttackSide::Side;

    const Vec2i f = facingTileDelta(defender.getFacing());
    const Vec2i p{f.y, -f.x}; // one perpendicular to the facing axis
    const int along = std::abs(delta.x * f.x + delta.y * f.y);
    const int perp = std::abs(delta.x * p.x + delta.y * p.y);

    // Strictly of the side axis → not in the front/rear arcs.
    if (perp > along)
        return AttackSide::Side;

    // Lying on the facing axis: which way relative to the defender?
    const int forward = delta.x * f.x + delta.y * f.y;
    return forward >= 0 ? AttackSide::Front : AttackSide::Rear;
}

bool CombatSystem::rollHit(const HitContext &ctx)
{
    if (ctx.neverMiss)
        return true;

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
    int offense = ctx.basePower
                  + (ctx.isMagical ? atk.getMagic() + ctx.tileMagicBonus
                                   : atk.getAttack() + ctx.tileAttackBonus);

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
    int offense = ctx.basePower
                  + (ctx.isMagical ? atk.getMagic() + ctx.tileMagicBonus
                                   : atk.getAttack() + ctx.tileAttackBonus);
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
