#include "battle/combat/SkillEffects.h"

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
               hasEffect(skill, SkillEffectType::Debuff);
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

    std::string describeSkill(const SkillData &skill, const HitContext &ctx)
    {
        if (hasEffect(skill, SkillEffectType::Heal) && !isOffensive(skill))
        {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "HEAL +%d", computeHealAmount(ctx));
            return buf;
        }
        if (hasEffect(skill, SkillEffectType::Buff) ||
            hasEffect(skill, SkillEffectType::Debuff))
        {
            const StatKind stat = statKindFromKey(skill.statKey);
            char buf[48];
            std::snprintf(buf, sizeof(buf), "%s %+d (%dT)",
                          statKindLabel(stat), skill.statAmount, skill.buffTurns);
            return buf;
        }
        return describeAttack(ctx);
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
} // namespace skill_effects