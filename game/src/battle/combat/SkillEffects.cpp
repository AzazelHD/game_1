#include "battle/combat/SkillEffects.h"

#include "data/SkillLoader.h"
#include "battle/unit/StatusEffect.h"

#include <algorithm>
#include <cstdio>

namespace
{
    // Offense preview text for a damage skill/basic attack: preview already
    // applies affinity + defense and reports the hit chance.
    std::string describeDamage(const HitContext &ctx)
    {
        const CombatResult preview = CombatSystem::preview(ctx);
        char buf[48];
        std::snprintf(buf, sizeof(buf), "DMG %d ACC %d%%",
                      preview.damage, static_cast<int>(preview.hitChance + 0.5f));
        return buf;
    }
} // namespace

namespace skill_effects
{
    bool hasEffect(const SkillData &skill, SkillEffectType type)
    {
        for (SkillEffectType t : skill.effectTypes)
            if (t == type)
                return true;
        return false;
    }

    bool isSupportive(const SkillData &skill)
    {
        const bool supportive = hasEffect(skill, SkillEffectType::Heal) ||
                                hasEffect(skill, SkillEffectType::Buff);
        const bool offensive = hasEffect(skill, SkillEffectType::Damage) ||
                               hasEffect(skill, SkillEffectType::Debuff);
        return supportive && !offensive;
    }

    bool isOffensive(const SkillData &skill)
    {
        return hasEffect(skill, SkillEffectType::Damage) ||
               hasEffect(skill, SkillEffectType::Debuff) ||
               hasEffect(skill, SkillEffectType::Ko);
    }

    StatKind statKindFromKey(const std::string &key)
    {
        if (key == "magic")
            return StatKind::Magic;
        if (key == "magicDefense" || key == "magicDef")
            return StatKind::MagicDefense;
        if (key == "defense" || key == "def")
            return StatKind::Defense;
        if (key == "speed")
            return StatKind::Speed;
        if (key == "evasion")
            return StatKind::Evasion;
        return StatKind::Attack; // "attack" (and empty/unknown → attack)
    }

    const char *statKindLabel(StatKind stat)
    {
        switch (stat)
        {
        case StatKind::Magic:
            return "MAG";
        case StatKind::MagicDefense:
            return "MDEF";
        case StatKind::Defense:
            return "DEF";
        case StatKind::Speed:
            return "SPD";
        case StatKind::Evasion:
            return "EVA";
        case StatKind::Attack:
        default:
            return "ATK";
        }
    }

    int computeHealAmount(const HitContext &ctx)
    {
        if (!ctx.attacker)
            return ctx.basePower;
        return std::max(0, ctx.basePower + ctx.attacker->getMagic());
    }

    // ── The damage preview ───────────────────────────────────────────────────
    // Deliberately narrow: damage and accuracy and nothing else. What a skill
    // *causes* (element, statuses, stat mods) lives in describeSkillDetails(),
    // which the confirm window shows, so the same fact is never printed twice.
    std::string describeSkill(const SkillData &skill, const HitContext &ctx)
    {
        if (hasEffect(skill, SkillEffectType::Damage))
            return describeAttack(ctx);

        // Any skill without a damage effect - heal, buff, debuff, and a KO
        // (which is a lethal rider, so it rolls just like a stun would):
        // "---" in the damage slot, but the accuracy it will actually roll.
        char buf[32];
        std::snprintf(buf, sizeof(buf), "--- ACC %d%%",
                      static_cast<int>(CombatSystem::preview(ctx).hitChance + 0.5f));
        return buf;
    }

    std::string describeAttack(const HitContext &ctx)
    {
        return describeDamage(ctx);
    }

    std::string describeStatMod(const SkillData &skill)
    {
        const StatKind stat = statKindFromKey(skill.statKey);
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%s %+d", statKindLabel(stat), skill.statAmount);
        return buf;
    }

    // The riders a skill attaches on top of its own description: the stat mod
    // it applies and the statuses it inflicts. These are the facts the DMG/ACC
    // preview has no room for.
    std::string describeSkillRiders(const SkillData &skill)
    {
        std::string riders;
        char buf[48];

        if (!skill.statKey.empty() && skill.buffTurns > 0)
        {
            std::snprintf(buf, sizeof(buf), "%s %+d for %d turn%s",
                          statKindLabel(statKindFromKey(skill.statKey)),
                          skill.statAmount, skill.buffTurns,
                          skill.buffTurns == 1 ? "" : "s");
            riders += buf;
        }

        if (!skill.statuses.empty())
        {
            if (!riders.empty())
                riders += " ";
            riders += "Causes:";
            for (const StatusEffectApply &apply : skill.statuses)
            {
                const StatusInfo &info = statusInfo(apply.type);
                std::snprintf(buf, sizeof(buf), " %s", info.label);
                riders += buf;
                if (apply.damagePerTick > 0)
                {
                    std::snprintf(buf, sizeof(buf), " %d/turn", apply.damagePerTick);
                    riders += buf;
                }
                if (apply.turns > 0)
                {
                    std::snprintf(buf, sizeof(buf), " (%d turn%s)",
                                  apply.turns, apply.turns == 1 ? "" : "s");
                    riders += buf;
                }
                if (apply.chance < 100)
                {
                    std::snprintf(buf, sizeof(buf), " %d%%", apply.chance);
                    riders += buf;
                }
            }
        }

        return riders;
    }

    std::string describeSkillDetail(const SkillData &skill)
    {
        const std::string riders = describeSkillRiders(skill);
        if (riders.empty())
            return skill.description;
        if (skill.description.empty())
            return riders;
        return skill.description + " " + riders;
    }
} // namespace skill_effects