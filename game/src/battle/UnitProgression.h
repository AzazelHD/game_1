#pragma once

// UnitProgression centralizes unit progression metadata:
// - race static bonuses (base stat modifiers + elemental affinities)
// - gender static bonuses
// - race growth curves used during level-up
// This keeps UnitData focused on raw unit identity/base stats.

#include <vector>

#include "battle/UnitData.h"

struct RaceData
{
    Race race = Race::Human;
    std::vector<ElementAffinity> affinities;
    int bonusMaxHp = 0;
    int bonusMaxMp = 0;
    int bonusAttack = 0;
    int bonusDefense = 0;
    int bonusMagic = 0;
    int bonusMagicDefense = 0;
    int bonusMoveRange = 0;
    int bonusAtkRange = 0;
    int bonusEvasion = 0;
    int bonusSpeed = 0;
};

struct GenderData
{
    Gender gender = Gender::None;
    int bonusMaxHp = 0;
    int bonusMaxMp = 0;
    int bonusAttack = 0;
    int bonusDefense = 0;
    int bonusMagic = 0;
    int bonusMagicDefense = 0;
    int bonusMoveRange = 0;
    int bonusAtkRange = 0;
    int bonusEvasion = 0;
    int bonusSpeed = 0;
};

struct RaceGrowth
{
    Race race = Race::Human;
    float hpPerLevel = 0.0f;
    float mpPerLevel = 0.0f;
    float attackPerLevel = 0.0f;
    float defensePerLevel = 0.0f;
    float magicPerLevel = 0.0f;
    float magicDefPerLevel = 0.0f;
    float speedPerLevel = 0.0f;
    float evasionPerLevel = 0.0f;
};

const RaceData &getRaceData(Race race);
const GenderData &getGenderData(Gender gender);
const RaceGrowth &getRaceGrowth(Race race);
