#pragma once

#include <string>
#include <vector>

enum class Gender
{
    Male,
    Female,
    None
};

enum class Race
{
    Human,
    Elf,
    Undead
};

enum class Element
{
    Fire,
    Ice,
    Lightning,
    Holy,
    Dark,
    Neutral,
};

enum class Affinity
{
    Weak,
    Neutral,
    Resistant,
    Immune,
    Absorb
};

struct ElementAffinity
{
    Element element;
    Affinity affinity;
};

struct UnitData
{
    std::string name;
    std::string className;
    std::string spriteSetId;
    int acquisitionIndex = 0;
    Race race;
    Gender gender;
    int level = 1;
    int maxHp = 30;
    int maxMp = 8;
    int attack = 10;
    int defense = 5;
    int magic = 5;
    int magicDefense = 5;
    int moveRange = 4;
    int atkRange = 1;
    int evasion = 10;
    int jump = 1;
    int speed = 25;
    int team = 0;
    std::vector<ElementAffinity> affinities;
    std::vector<std::string> skillIds;
};

struct StatBonuses
{
    int speed = 0;
    int attack = 0;
    int defense = 0;
    int magic = 0;
    int magicDef = 0;
    int evasion = 0;
    int moveRange = 0;
    int atkRange = 0;
    int maxHp = 0;
    int maxMp = 0;
};

enum class ActionType
{
    Move,
    Attack,
    Item,
    Defend,
    Wait,
};

enum class SkillType
{
    Foo_Human,
    Foo_Elf,
    Foo_Undead,
};

enum class UnitState
{
    Idle,
    Selected,
    Moving,
    Attacking,
    Done,
    Dead
};

inline const char *toString(Race race)
{
    switch (race)
    {
    case Race::Human:
        return "Human";
    case Race::Elf:
        return "Elf";
    case Race::Undead:
        return "Undead";
    }
    return "Unknown";
}

inline const char *toString(Gender gender)
{
    switch (gender)
    {
    case Gender::Male:
        return "Male";
    case Gender::Female:
        return "Female";
    case Gender::None:
        return "None";
    }
    return "Unknown";
}

inline const char *toString(Element element)
{
    switch (element)
    {
    case Element::Fire:
        return "Fire";
    case Element::Ice:
        return "Ice";
    case Element::Lightning:
        return "Lightning";
    case Element::Holy:
        return "Holy";
    case Element::Dark:
        return "Dark";
    case Element::Neutral:
        return "Neutral";
    }
    return "Unknown";
}

inline const char *toString(Affinity affinity)
{
    switch (affinity)
    {
    case Affinity::Weak:
        return "Weak";
    case Affinity::Neutral:
        return "Neutral";
    case Affinity::Resistant:
        return "Resistant";
    case Affinity::Immune:
        return "Immune";
    case Affinity::Absorb:
        return "Absorb";
    }
    return "Unknown";
}
