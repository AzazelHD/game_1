#include "battle/UnitProgression.h"

#include <unordered_map>

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
        .race = Race::Elf,
        .bonusMaxHp = 0,
        .bonusMaxMp = 5,
        .bonusAttack = 0,
        .bonusDefense = 0,
        .bonusMagic = 5,
        .bonusMagicDefense = 0,
        .bonusMoveRange = 0,
        .bonusAtkRange = 0,
        .bonusEvasion = 5,
        .bonusSpeed = 10,
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
        .race = Race::Elf,
        .hpPerLevel = 1.6f,
        .mpPerLevel = 1.1f,
        .attackPerLevel = 0.5f,
        .defensePerLevel = 0.5f,
        .magicPerLevel = 0.9f,
        .magicDefPerLevel = 0.8f,
        .speedPerLevel = 0.6f,
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
        {Race::Undead, &kUndeadGrowth},
    };
    return *table.at(race);
}
