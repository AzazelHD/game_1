#include "inventory/Item.h"

#include <nlohmann/json.hpp>

bool isValidItemDefinition(const ItemDefinition &definition)
{
    return !definition.key.empty() &&
           !definition.displayName.empty() &&
           definition.maxStackSize > 0;
}

nlohmann::json itemStackToJson(const ItemStack &stack)
{
    return nlohmann::json{
        {"itemId", stack.itemId},
        {"quantity", stack.quantity},
    };
}

bool itemStackFromJson(const nlohmann::json &value, ItemStack &outStack)
{
    if (!value.is_object() ||
        !value.contains("itemId") ||
        !value.contains("quantity") ||
        !value.at("itemId").is_number_unsigned() ||
        !value.at("quantity").is_number_unsigned())
        return false;

    const std::uint64_t itemId = value.at("itemId").get<std::uint64_t>();
    const std::uint64_t quantity = value.at("quantity").get<std::uint64_t>();
    if (itemId > UINT16_MAX || quantity == 0 || quantity > UINT32_MAX)
        return false;

    outStack.itemId = static_cast<ItemId>(itemId);
    outStack.quantity = static_cast<std::uint32_t>(quantity);
    return true;
}