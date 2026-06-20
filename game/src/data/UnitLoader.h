#pragma once
#include <string>
#include <vector>

struct UnitData;

// UnitLoader reads unit definition JSON files and returns UnitData structs.
//
// Expected JSON schema (one file per unit type):
//   "name":         "Aza",
//   "race":         "Human",
//   "gender":       "Male",
//   "level":        1,
//   "maxHp":        30,
//   "maxMp":        8,
//   "attack":       10,
//   "defense":      5,
//   "magic":        5,
//   "magicDefense": 5,
//   "moveRange":    4,
//   "atkRange":     1,
//   "evasion":      10,
//   "speed":        25,
//   "team":         0
//
// [x]: Implement:
//   static UnitData load(const std::string& filePath)
//     Parse the JSON, map fields to UnitData members, return it.
//   static std::vector<UnitData> loadAll(const std::string& directory)
//     Load every .json file in a directory. Useful for loading a full roster.
class UnitLoader
{
public:
    static UnitData load(const std::string &filePath);
    static std::vector<UnitData> loadAll(const std::string &directory);
};
