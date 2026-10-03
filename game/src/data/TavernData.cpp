#include "data/TavernData.h"

#include <nlohmann/json.hpp>

#include <fstream>

using json = nlohmann::json;

std::vector<TavernCandidate> TavernData::loadCandidates(const std::string &filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
        return {};

    const json j = json::parse(file);

    std::vector<TavernCandidate> candidates;
    if (j.contains("candidates") && j["candidates"].is_array())
    {
        for (const auto &entry : j["candidates"])
        {
            if (!entry.is_object())
                continue;
            TavernCandidate candidate;
            if (entry.contains("templatePath"))
                candidate.templatePath = entry.value("templatePath", "");
            if (entry.contains("customName"))
                candidate.customName = entry.value("customName", "");
            if (entry.contains("cost"))
                candidate.cost = entry.value("cost", 0);
            if (entry.contains("flavor"))
                candidate.flavor = entry.value("flavor", "");
            candidates.push_back(std::move(candidate));
        }
    }
    return candidates;
}

std::vector<TavernLine> TavernData::loadLore(const std::string &filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
        return {};

    const json j = json::parse(file);

    std::vector<TavernLine> lines;
    if (j.contains("lines") && j["lines"].is_array())
    {
        for (const auto &entry : j["lines"])
        {
            if (!entry.is_object())
                continue;
            TavernLine line;
            if (entry.contains("speaker"))
                line.speaker = entry.value("speaker", "");
            if (entry.contains("text"))
                line.text = entry.value("text", "");
            lines.push_back(std::move(line));
        }
    }
    return lines;
}