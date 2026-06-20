#pragma once
#include <string>
#include <vector>
#include "engine/math/Vec2.h"

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
    int speed = 25;
    int team = 0;
    std::vector<ElementAffinity> affinities;
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
    Wait,
    Defend, // TODO: requires shield equipped
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