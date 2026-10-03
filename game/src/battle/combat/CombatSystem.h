#pragma once

#include "battle/unit/UnitData.h"

#include <string>
#include <random>

class Unit;
class BattleMap;
struct SkillData;

enum class AttackSide
{
    Front, // +0 accuracy, -damage multiplier
    Side,  // +10 accuracy
    Rear,  // +20 accuracy, +damage multiplier
};

// CombatResult carries everything that happened in one attack resolution.
struct CombatResult
{
    int damage = 0;
    bool hit = false;
    bool crit = false;
    bool absorbed = false;
    Affinity affinityResult = Affinity::Neutral;
    float hitChance = 0.0f;
};

// HitContext carries all the info needed to resolve an attack. It is constructed by the caller (BattleState) based on the attacker, target, and chosen skill/action.
struct HitContext
{
    const Unit *attacker = nullptr;
    const Unit *target = nullptr;

    int basePower = 0; // from skill or weapon
    bool isMagical = false;
    Element element = Element::Neutral;

    int skillAccuracy = 0; // base hit % from skill/weapon
    AttackSide side = AttackSide::Front;
    // Terrain bonuses, filled by applyTileBonuses(): the attacker's ground
    // (grass_atk / grass_mag) adds flat offense, the defender's ground
    // (grass_eva) adds evasion.
    int tileAttackBonus = 0;
    int tileMagicBonus = 0;
    int tileEvasionBonus = 0; // from target's tile (e.g. grass)

    // From the skill's debug flag (SkillData::neverMiss). Kept on the context
    // rather than special-cased at call sites so resolve(), preview() and the
    // AI all behave identically.
    bool neverMiss = false; // ignores evasion + facing + terrain evasion; always hits
};

// CombatSystem resolves combat. It is a stateless utility — no member variables.
// All methods are static (or it can be a namespace instead of a class).
//
// [x]: Implement:
//   static CombatResult resolve(const Unit& attacker, const Unit& target)
//     1. Roll hit: compare attacker accuracy vs target evasion (simple random check).
//        If miss, return CombatResult{ 0, false, false }.
//     2. Roll crit: small random chance (e.g. 10%). Crit doubles damage.
//     3. Compute damage: attacker.attack - target.defense + small random variance.
//        Clamp to 0 minimum (never heal from an attack).
//     4. Return the filled CombatResult.
//
// Tip: use std::rand() for now. Replace with a seeded mt19937 later for reproducibility.
//
// The caller (BattleState) is responsible for calling target.takeDamage(result.damage).
class CombatSystem
{
public:
    static CombatResult resolve(const HitContext &ctx);
    static CombatResult preview(const HitContext &ctx);

    // Shared by every camp so a plain basic attack resolves IDENTICALLY for
    // player and enemy units: damage = attacker stat − target defense ±10%
    // variance, 95 base acc. The side multiplier/accuracy is derived from
    // the attacker's position relative to the target's facing.
    // Thin wrapper over hitContextFor(SkillData::makeBasicAttack(), ...), so it
    // stays identical to the in-game basic-attack skill by construction.
    static HitContext basicAttackContext(const Unit &attacker, const Unit &target);

    // Single source of truth for turning a SkillData into a HitContext — used
    // by the player path (BattleState::makeHitContext), the AI
    // (EnemyAI::resolveAttack) and the damage preview, so all three share
    // identical stat-scaling and flanking rules. Does NOT apply terrain; the
    // caller still calls applyTileBonuses().
    static HitContext hitContextFor(const SkillData &skill, const Unit &attacker, const Unit &target);

    // Maps the attacker's tile position relative to the *defender's* facing
    // to an AttackSide: standing inside the defender's facing cone is Front
    // (guarded), directly opposite is Rear, everything else is Side. Works
    // at any range. Returns Side for a same-tile (degenerate) case.
    static AttackSide attackSideFrom(const Unit &attacker, const Unit &defender);

    // Adds terrain class bonuses from the attacker's / defender's tile into
    // ctx (grass_atk -> +tileAttackBonus, grass_mag -> +tileMagicBonus on the
    // attacker side; grass_eva -> +tileEvasionBonus on the defender side).
    // Guards invalid/zeroed tiles so callers never need to check map bounds.
    static void applyTileBonuses(HitContext &ctx, const BattleMap &map);

    // True with probability percent/100. percent >= 100 always true, <= 0
    // never. Used for status-effect application chances.
    static bool chanceRoll(int percent);

    // Terrain bonus magnitudes (flat)
    static constexpr int TILE_ATK_BONUS = 2;
    static constexpr int TILE_MAG_BONUS = 3;
    static constexpr int TILE_EVA_BONUS = 5;

    // Tunable constants
    static constexpr float CRIT_CHANCE = 0.10f;
    static constexpr float CRIT_MULTIPLIER = 1.50f;
    static constexpr float FRONT_DMG_MULTIPLIER = 0.90f;
    static constexpr float REAR_DMG_MULTIPLIER = 1.10f;

    static constexpr float AFFINITY_WEAK = 1.50f;
    static constexpr float AFFINITY_RESIST = 0.50f;
    static constexpr float AFFINITY_IMMUNE = 0.00f;

private:
    static std::mt19937 &rng();
    static bool rollHit(const HitContext &ctx);
    static bool rollCrit();
    static int computeDamage(const HitContext &ctx);
    // Shared by resolve() and preview(): basePower + attacker stat, with the
    // AttackSide multiplier applied. No variance, no crit, no defense/affinity
    // yet — those differ between resolve() (adds variance+crit) and preview()
    // (neither), so they stay in each caller.
    static int computeBaseOffense(const HitContext &ctx);
    // Applies element affinity to `offense` in place, and records the result
    // (and, for Absorb, sets result.absorbed) into `outResult`. Shared verbatim
    // between resolve() and preview() — the affinity switch was identical in both.
    static void applyAffinity(const HitContext &ctx, int &offense, CombatResult &outResult);
    static Affinity getAffinity(const Unit &target, Element element);
    static int sideAccuracyBonus(AttackSide side);
    static int applySideMultiplier(int offense, AttackSide side);
    static int rollVariance(int variance);
};
