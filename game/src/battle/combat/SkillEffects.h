#pragma once

#include "battle/combat/CombatSystem.h"
#include "data/SkillLoader.h"
#include "battle/unit/Unit.h"

#include <string>

// Effect classification + resolution helpers shared by menu/UI targeting and
// attack resolution. Skills are either offensive (damage/debuff — target
// enemies) or supportive (heal/buff — target allies/self); a skill carrying
// both is treated as offensive (its buff/debuff rides along with the damage).
//
// [x] Classify a skill by its effectTypes:
namespace skill_effects
{
    bool hasEffect(const SkillData &skill, SkillEffectType type);

    // True if the skill heals or buffs (and has no offensive part).
    bool isSupportive(const SkillData &skill);

    // True if the skill damages or debuffs (default for a damage-only skill).
    bool isOffensive(const SkillData &skill);

    // Maps a skill's `statKey` JSON string to a StatKind. Unknown/empty keys
    // fall back to StatKind::Attack.
    StatKind statKindFromKey(const std::string &key);

    // Display label for a StatKind ("ATK", "DEF", ...).
    const char *statKindLabel(StatKind stat);

    // Amount healed by a heal skill: basePower + caster magic (flat, no roll).
    int computeHealAmount(const HitContext &ctx);

    // One-line effect preview used by the top battle text / damage preview:
    //   DMG 12 ACC 92%   |   HEAL +14   |   ATK +3 (3T)   |   ATK -2 (3T)
    std::string describeSkill(const SkillData &skill, const HitContext &ctx);

    // Basic-attack preview text ("DMG 12 ACC 92%").
    std::string describeAttack(const HitContext &ctx);

    // Short floating-text token for a buff/debuff skill ("ATK +3", "ATK -2").
    std::string describeStatMod(const SkillData &skill);
} // namespace skill_effects