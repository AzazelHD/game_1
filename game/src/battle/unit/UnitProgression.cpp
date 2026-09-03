#include "battle/unit/UnitProgression.h"

#include <unordered_map>
#include <array>

namespace
{
    const RaceData kHumanRaceData{
        .race = Race::Human,
        .bonusMaxHp = 0,
        .bonusMaxMp = 0,
        .bonusAttack = 0,
        .bonusDefense = 0,
        .bonusMagic = 0,
        .bonusMagicDefense = 0,
        .bonusMoveRange = 0,
        .bonusAtkRange = 0,
        .bonusEvasion = 0,
        .bonusSpeed = 0,
    };

    const RaceData kElfRaceData{
        // PLACEHOLDER BALANCE: values express RACE_DESIGN's magic/speed/
        // evasion bias and lower HP/defense direction; rebalance later.
        .race = Race::Elf,
        .bonusMaxHp = -2,
        .bonusMaxMp = 5,
        .bonusAttack = 0,
        .bonusDefense = -1,
        .bonusMagic = 5,
        .bonusMagicDefense = 0,
        .bonusMoveRange = 0,
        .bonusAtkRange = 0,
        .bonusEvasion = 5,
        .bonusSpeed = 10,
    };

    const RaceData kElinRaceData{
        // PLACEHOLDER BALANCE: RACE_DESIGN specifies a fast, evasive,
        // magic-capable glass-cannon profile but no final numeric values.
        .race = Race::Elin,
        .bonusMaxHp = -4,
        .bonusMaxMp = 3,
        .bonusAttack = 0,
        .bonusDefense = -2,
        .bonusMagic = 2,
        .bonusMagicDefense = 0,
        .bonusMoveRange = 0,
        .bonusAtkRange = 0,
        .bonusEvasion = 8,
        .bonusSpeed = 12,
    };

    const RaceData kUndeadRaceData{
        .race = Race::Undead,
        .affinities = {
            {Element::Holy, Affinity::Weak},
            {Element::Dark, Affinity::Absorb},
        },
        .bonusMaxHp = 10,
        .bonusMaxMp = 0,
        .bonusAttack = -2,
        .bonusDefense = 0,
        .bonusMagic = 0,
        .bonusMagicDefense = 5,
        .bonusMoveRange = 0,
        .bonusAtkRange = 0,
        .bonusEvasion = -5,
        .bonusSpeed = 0,
    };

    const RaceGrowth kHumanGrowth{
        .race = Race::Human,
        .hpPerLevel = 2.0f,
        .mpPerLevel = 0.8f,
        .attackPerLevel = 0.7f,
        .defensePerLevel = 0.6f,
        .magicPerLevel = 0.6f,
        .magicDefPerLevel = 0.6f,
        .speedPerLevel = 0.5f,
    };

    const RaceGrowth kElfGrowth{
        // PLACEHOLDER BALANCE: directional Elf bias from RACE_DESIGN.
        .race = Race::Elf,
        .hpPerLevel = 1.6f,
        .mpPerLevel = 1.1f,
        .attackPerLevel = 0.5f,
        .defensePerLevel = 0.5f,
        .magicPerLevel = 0.9f,
        .magicDefPerLevel = 0.8f,
        .speedPerLevel = 0.6f,
    };

    const RaceGrowth kElinGrowth{
        // PLACEHOLDER BALANCE: directional Elin bias from RACE_DESIGN.
        .race = Race::Elin,
        .hpPerLevel = 1.4f,
        .mpPerLevel = 0.9f,
        .attackPerLevel = 0.6f,
        .defensePerLevel = 0.4f,
        .magicPerLevel = 0.7f,
        .magicDefPerLevel = 0.6f,
        .speedPerLevel = 0.8f,
        .evasionPerLevel = 0.2f,
    };

    const RaceGrowth kUndeadGrowth{
        .race = Race::Undead,
        .hpPerLevel = 2.4f,
        .mpPerLevel = 0.4f,
        .attackPerLevel = 0.8f,
        .defensePerLevel = 0.7f,
        .magicPerLevel = 0.4f,
        .magicDefPerLevel = 0.9f,
        .speedPerLevel = 0.4f,
    };

    const ClassGrowth kSoldierGrowth{
        .baseClass = BaseClass::Soldier,
        .promotion = PromotionClass::None,
    };

    // Placeholder balance values for the promotion biases described in
    // RACE_DESIGN.md. Replace when the class balance pass is designed.
    const ClassGrowth kTemplarGrowth{
        .baseClass = BaseClass::Soldier,
        .promotion = PromotionClass::Templar,
        .mpPerLevel = 0.2f,
        .magicPerLevel = 0.1f,
        .magicDefPerLevel = 0.2f,
    };
    const ClassGrowth kKnightGrowth{
        .baseClass = BaseClass::Soldier,
        .promotion = PromotionClass::Knight,
        .hpPerLevel = 0.4f,
        .defensePerLevel = 0.2f,
    };
    const ClassGrowth kPaladinGrowth{
        .baseClass = BaseClass::Soldier,
        .promotion = PromotionClass::Paladin,
        .hpPerLevel = 0.2f,
        .mpPerLevel = 0.1f,
        .attackPerLevel = 0.1f,
        .magicPerLevel = 0.1f,
    };

    const GenderData kMaleGenderData{
        .gender = Gender::Male,
    };
    const GenderData kFemaleGenderData{
        .gender = Gender::Female,
    };
    const GenderData kNoneGenderData{
        .gender = Gender::None,
    };

}

const RaceData &getRaceData(Race race)
{
    static const std::unordered_map<Race, const RaceData *> table{
        {Race::Human, &kHumanRaceData},
        {Race::Elf, &kElfRaceData},
        {Race::Elin, &kElinRaceData},
        {Race::Undead, &kUndeadRaceData},
    };
    return *table.at(race);
}

const GenderData &getGenderData(Gender gender)
{
    static const std::unordered_map<Gender, const GenderData *> table{
        {Gender::Male, &kMaleGenderData},
        {Gender::Female, &kFemaleGenderData},
        {Gender::None, &kNoneGenderData},
    };
    return *table.at(gender);
}

const RaceGrowth &getRaceGrowth(Race race)
{
    static const std::unordered_map<Race, const RaceGrowth *> table{
        {Race::Human, &kHumanGrowth},
        {Race::Elf, &kElfGrowth},
        {Race::Elin, &kElinGrowth},
        {Race::Undead, &kUndeadGrowth},
    };
    return *table.at(race);
}

const ClassGrowth &getClassGrowth(BaseClass baseClass, PromotionClass promotion)
{
    static const std::array<ClassGrowth, 4> table{
        kSoldierGrowth,
        kTemplarGrowth,
        kKnightGrowth,
        kPaladinGrowth,
    };

    switch (promotion)
    {
    case PromotionClass::Templar:
        return table[1];
    case PromotionClass::Knight:
        return table[2];
    case PromotionClass::Paladin:
        return table[3];
    case PromotionClass::None:
        return table[0];
    // PLACEHOLDER BALANCE: Elf/Elin promotion adjustments express only the
    // direction in RACE_DESIGN and must be replaced by a balance pass.
    case PromotionClass::Sharpshooter:
    {
        static const ClassGrowth growth{.baseClass = BaseClass::Archer, .promotion = PromotionClass::Sharpshooter, .attackPerLevel = 0.2f, .speedPerLevel = 0.1f};
        return growth;
    }
    case PromotionClass::Ranger:
    {
        static const ClassGrowth growth{.baseClass = BaseClass::Archer, .promotion = PromotionClass::Ranger, .attackPerLevel = 0.1f, .speedPerLevel = 0.2f};
        return growth;
    }
    case PromotionClass::Windrunner:
    {
        static const ClassGrowth growth{.baseClass = BaseClass::Archer, .promotion = PromotionClass::Windrunner, .speedPerLevel = 0.3f, .evasionPerLevel = 0.2f};
        return growth;
    }
    case PromotionClass::Elementalist:
    {
        static const ClassGrowth growth{.baseClass = BaseClass::Mage, .promotion = PromotionClass::Elementalist, .magicPerLevel = 0.3f};
        return growth;
    }
    case PromotionClass::Enchanter:
    {
        static const ClassGrowth growth{.baseClass = BaseClass::Mage, .promotion = PromotionClass::Enchanter, .mpPerLevel = 0.2f, .magicPerLevel = 0.1f};
        return growth;
    }
    case PromotionClass::Warden:
    {
        static const ClassGrowth growth{.baseClass = BaseClass::Mage, .promotion = PromotionClass::Warden, .defensePerLevel = 0.1f, .magicDefPerLevel = 0.3f};
        return growth;
    }
    case PromotionClass::Infiltrator:
    {
        static const ClassGrowth growth{.baseClass = BaseClass::Scout, .promotion = PromotionClass::Infiltrator, .attackPerLevel = 0.2f, .evasionPerLevel = 0.2f};
        return growth;
    }
    case PromotionClass::Trapper:
    {
        static const ClassGrowth growth{.baseClass = BaseClass::Scout, .promotion = PromotionClass::Trapper, .magicPerLevel = 0.1f, .speedPerLevel = 0.1f};
        return growth;
    }
    case PromotionClass::Pathfinder:
    {
        static const ClassGrowth growth{.baseClass = BaseClass::Scout, .promotion = PromotionClass::Pathfinder, .speedPerLevel = 0.2f};
        return growth;
    }
    case PromotionClass::BladeDancer:
    {
        static const ClassGrowth growth{.baseClass = BaseClass::Duelist, .promotion = PromotionClass::BladeDancer, .attackPerLevel = 0.2f, .speedPerLevel = 0.2f};
        return growth;
    }
    case PromotionClass::Assassin:
    {
        static const ClassGrowth growth{.baseClass = BaseClass::Duelist, .promotion = PromotionClass::Assassin, .attackPerLevel = 0.3f, .evasionPerLevel = 0.1f};
        return growth;
    }
    case PromotionClass::Fencer:
    {
        static const ClassGrowth growth{.baseClass = BaseClass::Duelist, .promotion = PromotionClass::Fencer, .defensePerLevel = 0.1f, .speedPerLevel = 0.2f};
        return growth;
    }
    }
    return table[0];
}
