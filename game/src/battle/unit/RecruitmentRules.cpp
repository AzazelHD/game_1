#include "battle/unit/RecruitmentRules.h"

bool isBaseClassEligible(Race race, BaseClass baseClass)
{
    switch (race)
    {
    case Race::Human:
        return baseClass == BaseClass::Soldier || baseClass == BaseClass::Archer || baseClass == BaseClass::Mage;
    case Race::Elf:
        return baseClass == BaseClass::Archer || baseClass == BaseClass::Mage || baseClass == BaseClass::Scout;
    case Race::Elin:
        return baseClass == BaseClass::Archer || baseClass == BaseClass::Mage || baseClass == BaseClass::Duelist;
    case Race::Undead:
        return false; // Deferred monster-unit type; not a playable recruit race.
    }
    return false;
}

std::vector<BaseClass> eligibleBaseClasses(Race race)
{
    switch (race)
    {
    case Race::Human:
        return {BaseClass::Soldier, BaseClass::Archer, BaseClass::Mage};
    case Race::Elf:
        return {BaseClass::Archer, BaseClass::Mage, BaseClass::Scout};
    case Race::Elin:
        return {BaseClass::Archer, BaseClass::Mage, BaseClass::Duelist};
    case Race::Undead:
        return {};
    }
    return {};
}

bool canPromote(BaseClass baseClass, PromotionClass promotion)
{
    const auto options = promotionOptions(baseClass);
    return std::find(options.begin(), options.end(), promotion) != options.end();
}

std::vector<PromotionClass> promotionOptions(BaseClass baseClass)
{
    switch (baseClass)
    {
    case BaseClass::Soldier:
        return {PromotionClass::Templar, PromotionClass::Knight, PromotionClass::Paladin};
    case BaseClass::Archer:
        return {PromotionClass::Sharpshooter, PromotionClass::Ranger, PromotionClass::Windrunner};
    case BaseClass::Mage:
        return {PromotionClass::Elementalist, PromotionClass::Enchanter, PromotionClass::Warden};
    case BaseClass::Scout:
        return {PromotionClass::Infiltrator, PromotionClass::Trapper, PromotionClass::Pathfinder};
    case BaseClass::Duelist:
        return {PromotionClass::BladeDancer, PromotionClass::Assassin, PromotionClass::Fencer};
    }
    return {};
}