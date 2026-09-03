#pragma once

#include <cstdint>
#include <string>

#include <nlohmann/json_fwd.hpp>

using ItemId = std::uint16_t;
inline constexpr std::uint32_t kDefaultItemStackSize = 99;

// Generic item metadata. Gear, consumables, and crafting materials can all
// use this definition without requiring the inventory to know their details.
struct ItemDefinition
{
    ItemId id = 0;
    std::string key;
    std::string displayName;
    std::string description;
    bool stackable = true;
    std::uint32_t maxStackSize = kDefaultItemStackSize;
};

struct ItemStack
{
    ItemId itemId = 0;
    std::uint32_t quantity = 0;
};

bool isValidItemDefinition(const ItemDefinition &definition);
nlohmann::json itemStackToJson(const ItemStack &stack);
bool itemStackFromJson(const nlohmann::json &value, ItemStack &outStack);