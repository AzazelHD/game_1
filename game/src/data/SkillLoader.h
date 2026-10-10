#pragma once

#include "battle/unit/UnitData.h"
#include "battle/unit/StatusEffect.h"
#include "battle/map/RangeRule.h"

#include <string>
#include <vector>

// What a skill does to its targets. A skill may carry several: a damage skill
// that also debuffs keeps both, and the order here is only used for display
// precedence in describeSkill.
//
//   Damage - offense minus defense, a real hit roll, "DMG n ACC n%".
//   Heal   - restores HP, applies unconditionally.
//   Buff   - beneficial stat/status rider.
//   Debuff - harmful stat/status rider.
//   Ko     - kills the target outright WITHOUT dealing damage. A KO has no
//            offense value, ignores defense, and shows "---" as its damage
//            readout, so it reads as a debuff that happens to be fatal rather
//            than as a huge number.
enum class SkillEffectType
{
    Damage,
    Heal,
    Buff,
    Debuff,
    Ko,
};

// A visual played on each target once the skill has connected, before the
// skill's consequence lands. This is the hook future skill animations hang
// off: a curse that needs its own flourish, a charge-up before a buff, a
// petrify sequence, and so on, all reuse it.
//
// An empty id means "no visual" and the consequence applies immediately, which
// is the same behavior every skill had before. A non-empty visual parks the
// turn timer for its duration (see AttackResolutionController), so the unit
// stays on screen and alive while the effect plays.
struct SkillVisual
{
    std::string id;       // combat animation id, e.g. "skull"
    float duration = 0.0f; // seconds to wait before the consequence applies

    [[nodiscard]] bool empty() const { return id.empty(); }
};

// Id of the shared basic-attack skill. Every unit can use it for free and it is
// the only skill that is not listed on a unit's skill set; the action menu
// offers it as its own "Attack" button rather than under Skills.
inline constexpr const char *kBasicAttackSkillId = "basic_attack";

// Wait applied when a skill names a `visualOnHit` without giving it a duration.
inline constexpr float kDefaultSkillVisualDuration = 1.0f;

// SkillData: a skill or ability a unit can use.
struct SkillData
{
    std::string id;          // unique key (e.g. "slash", "fireball")
    std::string name;        // display name
    std::string description; // flavour text (optional)

    // True for the shared basic-attack skill (see kBasicAttackSkillId). It
    // resolves exactly like any other skill — same targeting, damage and
    // resolution pipeline — but the action menu surfaces it as its own "Attack"
    // button, it costs no MP, and its reach/height come from the acting unit's
    // atkRange/jump instead of the authored `range`/`heightTolerance`.
    bool isBasicAttack = false;

    int basePower = 0;                        // flat damage before stats
    bool isMagical = false;                   // true = uses magic stat
    Element element = Element::Neutral;       // "neutral", "fire", "ice", etc.
    int skillAccuracy = 100;                  // base hit% (0–100)
    int range = 1;                            // tile reach
    int area = 0;                             // AoE radius (0 = single target)
    int mpCost = 0;                           // MP required to use
    std::vector<SkillEffectType> effectTypes; // "damage", "heal", "buff", "debuff", "ko"
    // Per skill level above 1, adds this much to basePower. Only meaningful
    // for damage/heal skills; the skill's learned level comes from
    // SkillProgression (player units) — units without progression (enemies)
    // always fight at the authored basePower. See BattleState::makeHitContext.
    int powerPerLevel = 0;

    // Buff/debuff parameters (ignored by pure damage/heal skills):
    // statKey   — which stat the mod applies to: "attack", "defense",
    //             "magic", "magicDefense", "speed", "evasion"
    // statAmount— signed flat amount (negative = debuff)
    // buffTurns — duration in the target's own turns
    std::string statKey;
    int statAmount = 0;
    int buffTurns = 0;

    // Status effects the skill inflicts (new optional `statuses` array in the
    // skill JSON, e.g. fireball's burn rider). Applied on a landed hit for
    // damage skills and on any supportive cast (heal/buff) target.
    // JSON schema per entry: { "type": "poison|burn|stun|sleep|barrier|
    //                                  attack_up|armor_down|attack_down",
    //                          "turns": N,
    //                          "damagePerTick": N (DoT statuses only),
    //                          "chance": 0–100 }
    std::vector<StatusEffectApply> statuses;

    // true  = cast animation plays ONCE before resolving all targets (e.g.
    //         a dragon's breath weapon) — TODO: play it in beginAttackResolution().
    // false = cast animation plays before EACH target (e.g. a mage's
    //         fire bolt jumping target to target) — TODO: play it per-step
    //         in processNextPendingResult().
    bool castOncePerArea = false;

    // Whether the skill's area effect lands on every target separately
    // (default; a fire bolt jumping target to target, effect→damage per unit)
    // or blooms ONCE over the whole blast area before each target's consequence
    // pops in turn (a shiva-style wide explosion: one burst, then N damage
    // numbers). Author `"effectPerTarget": false` on a skill to override.
    bool effectPerTarget = true;

    // ── Debug / scripted-assist flags ────────────────────────────────────────
    //
    // neverMiss: the attack always lands. Evasion, the defender's facing bonus
    // and terrain evasion are all bypassed, and the damage preview reports 100%
    // accuracy. For debugging and cutscene-style guaranteed hits only.
    bool neverMiss = false;

    // visualOnHit: when non-empty and the skill connects, this animation plays
    // on the target and the skill's consequence is only applied once it
    // finishes. See SkillVisual above.
    SkillVisual visualOnHit;

    // ── Range / targeting rules (backward-compatible, default = melee slash) ──
    int heightTolerance = 0;        // -1 = ignore height, 0 = same height only, N = within N
    int splashHeightTolerance = -1; // -1 = ignore (legacy), 0 = same height as center, N = within N
    bool requiresLineOfSight = false;
    bool splashRespectsWalls = true;

    // Flattens into the combat-facing RangeRule consumed by AttackRange.
    RangeRule toRangeRule() const;

    // Same, but resolves the acting unit's reach and height into the rule.
    // Every skill except the basic attack ignores those arguments and uses its
    // authored values; the basic attack takes reach from the unit's atkRange
    // and a height tolerance of (jump - 1) levels, so a melee unit is
    // same-height only and a leaper strikes across elevation.
    RangeRule toRangeRuleFor(int unitAtkRange, int unitJump) const;

    // Canonical in-code version of the basic-attack skill, used to seed the
    // skill database so the "Attack" button always resolves to a real skill
    // even if the assets directory could not be read.
    static SkillData makeBasicAttack();
};

// SkillLoader reads skill definitions from JSON.
//
// Expected JSON schema:
// {
//   "name": "Fireball",
//   "power": 15,
//   "range": 3,
//   "area": 1,
//   "effectTypes": ["damage"],
//   "mpCost": 5
// }
//
// TODO: Implement:
//   static SkillData load(const std::string& filePath)
//   static std::vector<SkillData> loadAll(const std::string& directory)
//
// After implementing, update CombatSystem::resolve() to accept a const SkillData* (nullable —
// nullptr means basic attack, non-null means skill use).
class SkillLoader
{
public:
    static SkillData load(const std::string &filePath);
    static std::vector<SkillData> loadAll(const std::string &directory);
};
