#pragma once

#include "battle/unit/UnitData.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

enum class ProgressionSkillCategory
{
    Active,
    Passive,
    Reaction,
};

struct ProgressionSkillDefinition
{
    std::string id;
    std::string name;
    ProgressionSkillCategory category = ProgressionSkillCategory::Active;
    int maxLevel = 1;
    std::vector<int> pointCosts;
    std::vector<BaseClass> eligibleBaseClasses;
    PromotionClass requiredPromotion = PromotionClass::None;
    bool placeholder = false;
    bool autoGranted = false;
};

const ProgressionSkillDefinition *findProgressionSkill(const std::string &skillId);
std::vector<const ProgressionSkillDefinition *> availableProgressionSkills(
    BaseClass baseClass, PromotionClass promotion);

class SkillProgression
{
public:
    SkillProgression() = default;
    SkillProgression(BaseClass baseClass, PromotionClass promotion);

    void setClass(BaseClass baseClass, PromotionClass promotion);

    BaseClass getBaseClass() const { return m_baseClass; }
    PromotionClass getPromotion() const { return m_promotion; }
    int activeEquipLimit() const { return m_promotion == PromotionClass::None ? 5 : 6; }
    int passiveEquipLimit() const { return 1; }
    int reactionEquipLimit() const { return 1; }

    bool canLearnOrUpgrade(const std::string &skillId, int availablePoints) const;
    bool learnOrUpgrade(const std::string &skillId, int &availablePoints);

    bool isLearned(const std::string &skillId) const;
    int levelOf(const std::string &skillId) const;
    bool isEquipped(const std::string &skillId) const;

    bool equip(const std::string &skillId);
    bool unequip(const std::string &skillId);

    const std::unordered_map<std::string, int> &learnedSkills() const { return m_levels; }
    const std::unordered_set<std::string> &equippedActive() const { return m_equippedActive; }
    const std::unordered_set<std::string> &equippedPassive() const { return m_equippedPassive; }
    const std::unordered_set<std::string> &equippedReaction() const { return m_equippedReaction; }

private:
    bool isEligible(const ProgressionSkillDefinition &skill) const;
    void grantPromotionSkills();
    std::unordered_set<std::string> &equippedFor(ProgressionSkillCategory category);
    const std::unordered_set<std::string> &equippedFor(ProgressionSkillCategory category) const;
    int equipLimitFor(ProgressionSkillCategory category) const;

    BaseClass m_baseClass = BaseClass::Soldier;
    PromotionClass m_promotion = PromotionClass::None;
    std::unordered_map<std::string, int> m_levels;
    std::unordered_set<std::string> m_equippedActive;
    std::unordered_set<std::string> m_equippedPassive;
    std::unordered_set<std::string> m_equippedReaction;
};