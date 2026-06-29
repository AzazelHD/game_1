#pragma once
#include <string>
#include "battle/UnitData.h"

class Unit;

enum class AttackSide
{
    Front, // +0 accuracy
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
    int tileEvasionBonus = 0; // from target's tile (e.g. grass)
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

    // Tunable constants
    static constexpr float CRIT_CHANCE = 0.10f;
    static constexpr float CRIT_MULTIPLIER = 1.50f;
    static constexpr float FRONT_DMG_MULTIPLIER = 0.90f;
    static constexpr float REAR_DMG_MULTIPLIER = 1.10f;

    static constexpr float AFFINITY_WEAK = 1.50f;
    static constexpr float AFFINITY_RESIST = 0.50f;
    static constexpr float AFFINITY_IMMUNE = 0.00f;

private:
    static bool rollHit(const HitContext &ctx);
    static bool rollCrit();
    static int computeDamage(const HitContext &ctx);
    static Affinity getAffinity(const Unit &target, Element element);
    static int sideAccuracyBonus(AttackSide side);
};