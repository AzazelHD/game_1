#include "battle/unit/SkillProgression.h"

#include <algorithm>
#include <array>

namespace
{
    // M4 progression table — REAL active skills from the combat skill
    // catalog (assets/skills/*.json). For Active skills the definition id is
    // the combat skill id, so a learned+equipped skill grafts directly onto
    // the battle unit's skill list.
    //
    // Recipes per base class:
    //   Soldier: slash, valor, barrier, weaken, sunder, venom_strike
    //   Mage:    fireball, heal, barrier, weaken, thunder_jolt
    //   Duelist: slash, weaken, sunder, venom_strike
    //   Archer:  valor, weaken
    //   Scout:   slash, fireball, valor, weaken, sunder, venom_strike, thunder_jolt
    const std::vector<ProgressionSkillDefinition> kSkills = {
        ProgressionSkillDefinition{
            .id = "slash",
            .name = "Slash",
            .description = "A basic physical attack (+3 power per level).",
            .category = ProgressionSkillCategory::Active,
            .maxLevel = 3,
            .pointCosts = {1, 1, 2},
            .eligibleBaseClasses = {BaseClass::Soldier, BaseClass::Duelist, BaseClass::Scout},
        },
        ProgressionSkillDefinition{
            .id = "fireball",
            .name = "Fireball",
            .description = "Launches a ball of fire (2x2 splash, +4 per level).",
            .category = ProgressionSkillCategory::Active,
            .maxLevel = 3,
            .pointCosts = {1, 1, 2},
            .eligibleBaseClasses = {BaseClass::Mage, BaseClass::Scout},
        },
        ProgressionSkillDefinition{
            .id = "heal",
            .name = "Heal",
            .description = "Restores an ally's HP (+4 power per level).",
            .category = ProgressionSkillCategory::Active,
            .maxLevel = 3,
            .pointCosts = {1, 1, 2},
            .eligibleBaseClasses = {BaseClass::Mage},
        },
        ProgressionSkillDefinition{
            .id = "valor",
            .name = "Valor",
            .description = "Raises an ally's attack.",
            .category = ProgressionSkillCategory::Active,
            .maxLevel = 1,
            .pointCosts = {1},
            .eligibleBaseClasses = {BaseClass::Soldier, BaseClass::Archer, BaseClass::Scout},
        },
        ProgressionSkillDefinition{
            .id = "weaken",
            .name = "Weaken",
            .description = "Lowers an enemy's attack (ATK- 3 turns).",
            .category = ProgressionSkillCategory::Active,
            .maxLevel = 1,
            .pointCosts = {1},
            .eligibleBaseClasses = {BaseClass::Mage, BaseClass::Duelist, BaseClass::Archer},
        },
        ProgressionSkillDefinition{
            .id = "barrier",
            .name = "Barrier",
            .description = "Raises an ally's armor & magic res (BRR 3 turns).",
            .category = ProgressionSkillCategory::Active,
            .maxLevel = 1,
            .pointCosts = {1},
            .eligibleBaseClasses = {BaseClass::Soldier, BaseClass::Mage},
        },
        ProgressionSkillDefinition{
            .id = "sunder",
            .name = "Sunder",
            .description = "Shatters an enemy's armor & magic res (ARM- 3 turns).",
            .category = ProgressionSkillCategory::Active,
            .maxLevel = 1,
            .pointCosts = {1},
            .eligibleBaseClasses = {BaseClass::Soldier, BaseClass::Duelist, BaseClass::Scout},
        },
        ProgressionSkillDefinition{
            .id = "venom_strike",
            .name = "Venom Strike",
            .description = "A poisoned slash (PSN 4/turn, 3 turns; +2 power per level).",
            .category = ProgressionSkillCategory::Active,
            .maxLevel = 3,
            .pointCosts = {1, 1, 2},
            .eligibleBaseClasses = {BaseClass::Soldier, BaseClass::Duelist, BaseClass::Scout},
        },
        ProgressionSkillDefinition{
            .id = "thunder_jolt",
            .name = "Thunder Jolt",
            .description = "Electrifies an enemy into a stun (1 turn; +2 power per level).",
            .category = ProgressionSkillCategory::Active,
            .maxLevel = 3,
            .pointCosts = {1, 1, 2},
            .eligibleBaseClasses = {BaseClass::Mage, BaseClass::Scout},
        },
    };

    bool containsBaseClass(const ProgressionSkillDefinition &skill, BaseClass baseClass)
    {
        return std::find(skill.eligibleBaseClasses.begin(),
                         skill.eligibleBaseClasses.end(),
                         baseClass) != skill.eligibleBaseClasses.end();
    }
}

const ProgressionSkillDefinition *findProgressionSkill(const std::string &skillId)
{
    for (const auto &skill : kSkills)
        if (skill.id == skillId)
            return &skill;
    return nullptr;
}

std::vector<const ProgressionSkillDefinition *> availableProgressionSkills(
    BaseClass baseClass, PromotionClass promotion)
{
    std::vector<const ProgressionSkillDefinition *> result;
    for (const auto &skill : kSkills)
    {
        if (containsBaseClass(skill, baseClass) &&
            (skill.requiredPromotion == PromotionClass::None ||
             skill.requiredPromotion == promotion))
            result.push_back(&skill);
    }
    return result;
}

SkillProgression::SkillProgression(BaseClass baseClass, PromotionClass promotion)
    : m_baseClass(baseClass), m_promotion(promotion)
{
    grantPromotionSkills();
}

void SkillProgression::setClass(BaseClass baseClass, PromotionClass promotion)
{
    m_baseClass = baseClass;
    m_promotion = promotion;
    grantPromotionSkills();
}

void SkillProgression::grantPromotionSkills()
{
    for (const auto &skill : kSkills)
    {
        if (skill.autoGranted && isEligible(skill))
            m_levels[skill.id] = skill.maxLevel;
    }
}

bool SkillProgression::isEligible(const ProgressionSkillDefinition &skill) const
{
    return containsBaseClass(skill, m_baseClass) &&
           (skill.requiredPromotion == PromotionClass::None ||
            skill.requiredPromotion == m_promotion);
}

int SkillProgression::levelOf(const std::string &skillId) const
{
    const auto it = m_levels.find(skillId);
    return it == m_levels.end() ? 0 : it->second;
}

bool SkillProgression::isLearned(const std::string &skillId) const
{
    return levelOf(skillId) > 0;
}

bool SkillProgression::canLearnOrUpgrade(const std::string &skillId, int availablePoints) const
{
    const ProgressionSkillDefinition *skill = findProgressionSkill(skillId);
    if (!skill || !isEligible(*skill))
        return false;

    if (skill->autoGranted)
        return false;

    const int currentLevel = levelOf(skillId);
    if (currentLevel >= skill->maxLevel)
        return false;

    const std::size_t costIndex = static_cast<std::size_t>(currentLevel);
    if (costIndex >= skill->pointCosts.size())
        return false;

    return availablePoints >= skill->pointCosts[costIndex];
}

bool SkillProgression::learnOrUpgrade(const std::string &skillId, int &availablePoints)
{
    if (!canLearnOrUpgrade(skillId, availablePoints))
        return false;

    const ProgressionSkillDefinition *skill = findProgressionSkill(skillId);
    const int currentLevel = levelOf(skillId);
    availablePoints -= skill->pointCosts[static_cast<std::size_t>(currentLevel)];
    m_levels[skillId] = currentLevel + 1;
    return true;
}

bool SkillProgression::isEquipped(const std::string &skillId) const
{
    const ProgressionSkillDefinition *skill = findProgressionSkill(skillId);
    return skill && equippedFor(skill->category).count(skillId) != 0;
}

int SkillProgression::equipLimitFor(ProgressionSkillCategory category) const
{
    switch (category)
    {
    case ProgressionSkillCategory::Active:
        return activeEquipLimit();
    case ProgressionSkillCategory::Passive:
        return passiveEquipLimit();
    case ProgressionSkillCategory::Reaction:
        return reactionEquipLimit();
    }
    return 0;
}

std::unordered_set<std::string> &SkillProgression::equippedFor(ProgressionSkillCategory category)
{
    switch (category)
    {
    case ProgressionSkillCategory::Active:
        return m_equippedActive;
    case ProgressionSkillCategory::Passive:
        return m_equippedPassive;
    case ProgressionSkillCategory::Reaction:
        return m_equippedReaction;
    }
    return m_equippedActive;
}

const std::unordered_set<std::string> &SkillProgression::equippedFor(ProgressionSkillCategory category) const
{
    switch (category)
    {
    case ProgressionSkillCategory::Active:
        return m_equippedActive;
    case ProgressionSkillCategory::Passive:
        return m_equippedPassive;
    case ProgressionSkillCategory::Reaction:
        return m_equippedReaction;
    }
    return m_equippedActive;
}

bool SkillProgression::equip(const std::string &skillId)
{
    const ProgressionSkillDefinition *skill = findProgressionSkill(skillId);
    if (!skill || !isEligible(*skill) || !isLearned(skillId))
        return false;

    auto &equipped = equippedFor(skill->category);
    if (equipped.count(skillId) != 0)
        return true;
    if (static_cast<int>(equipped.size()) >= equipLimitFor(skill->category))
        return false;

    equipped.insert(skillId);
    return true;
}

bool SkillProgression::unequip(const std::string &skillId)
{
    const ProgressionSkillDefinition *skill = findProgressionSkill(skillId);
    if (!skill)
        return false;
    return equippedFor(skill->category).erase(skillId) != 0;
}

void SkillProgression::restore(BaseClass baseClass, PromotionClass promotion,
                               const std::unordered_map<std::string, int> &learned,
                               const std::unordered_set<std::string> &equippedActive,
                               const std::unordered_set<std::string> &equippedPassive,
                               const std::unordered_set<std::string> &equippedReaction)
{
    m_baseClass = baseClass;
    m_promotion = promotion;
    m_levels = learned;
    m_equippedActive = equippedActive;
    m_equippedPassive = equippedPassive;
    m_equippedReaction = equippedReaction;
}