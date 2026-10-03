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

    // Merchant value in gold. The shop pays this when buying from the player
    // and charges a markup (see the shop implementation) when selling. 0 =
    // not sold by the shop.
    int goldValue = 0;
};

struct ItemStack
{
    ItemId itemId = 0;
    std::uint32_t quantity = 0;
};

bool isValidItemDefinition(const ItemDefinition &definition);
nlohmann::json itemStackToJson(const ItemStack &stack);
bool itemStackFromJson(const nlohmann::json &value, ItemStack &outStack);