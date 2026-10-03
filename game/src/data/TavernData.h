#pragma once

#include <string>
#include <vector>

// TavernCandidate describes a hireable unit offered at the tavern.
// Loaded from assets/tavern/candidates.json
struct TavernCandidate
{
    std::string templatePath;
    std::string customName;
    int cost = 0;
    std::string flavor;
};

// TavernLine is a single piece of bar-room dialogue.
// Loaded from assets/tavern/lore.json
struct TavernLine
{
    std::string speaker;
    std::string text;
};

// TavernData loads the static tavern candidate roster and bar chat lines.
class TavernData
{
public:
    static std::vector<TavernCandidate> loadCandidates(const std::string &filePath);
    static std::vector<TavernLine> loadLore(const std::string &filePath);
};