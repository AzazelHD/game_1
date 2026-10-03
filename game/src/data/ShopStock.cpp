#include "data/ShopStock.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>

using json = nlohmann::json;

std::vector<std::string> ShopStock::load(const std::string &filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
        return {};

    const json j = json::parse(file);

    std::vector<std::string> keys;
    if (j.contains("stock") && j["stock"].is_array())
    {
        for (const auto &entry : j["stock"])
        {
            if (entry.is_string())
                keys.push_back(entry.get<std::string>());
        }
    }
    return keys;
}