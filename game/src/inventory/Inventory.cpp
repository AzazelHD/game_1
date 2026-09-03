#include "inventory/Inventory.h"

#include <algorithm>
#include <limits>

#include <nlohmann/json.hpp>

Inventory::Inventory(std::uint32_t capacity)
    : m_capacity(std::min(capacity, kInventoryCapacity))
{
    m_stacks.reserve(64);
}

bool Inventory::contains(ItemId itemId) const
{
    return quantity(itemId) > 0;
}

std::uint32_t Inventory::quantity(ItemId itemId) const
{
    std::uint64_t total = 0;
    for (const ItemStack &stack : m_stacks)
    {
        if (stack.itemId == itemId)
            total += stack.quantity;
    }
    return static_cast<std::uint32_t>(std::min<std::uint64_t>(total, UINT32_MAX));
}

bool Inventory::has(ItemId itemId, std::uint32_t requestedQuantity) const
{
    return requestedQuantity > 0 && quantity(itemId) >= requestedQuantity;
}

bool Inventory::canAdd(const ItemDefinition &definition, std::uint32_t quantityToAdd) const
{
    if (!isValidItemDefinition(definition) || quantityToAdd == 0)
        return false;

    return quantityToAdd <= remainingCapacity();
}

bool Inventory::add(const ItemDefinition &definition, std::uint32_t quantityToAdd)
{
    if (!canAdd(definition, quantityToAdd))
        return false;

    const std::uint32_t stackLimit = definition.stackable ? definition.maxStackSize : 1;
    std::uint32_t remaining = quantityToAdd;

    if (definition.stackable)
    {
        for (ItemStack &stack : m_stacks)
        {
            if (stack.itemId != definition.id || stack.quantity >= stackLimit)
                continue;

            const std::uint32_t room = stackLimit - stack.quantity;
            const std::uint32_t added = std::min(room, remaining);
            stack.quantity += added;
            remaining -= added;
            if (remaining == 0)
                break;
        }
    }

    while (remaining > 0)
    {
        const std::uint32_t added = std::min(stackLimit, remaining);
        m_stacks.push_back(ItemStack{.itemId = definition.id, .quantity = added});
        remaining -= added;
    }

    m_usedCapacity += quantityToAdd;
    return true;
}

bool Inventory::remove(ItemId itemId, std::uint32_t quantityToRemove)
{
    if (!has(itemId, quantityToRemove))
        return false;

    std::uint32_t remaining = quantityToRemove;
    for (std::size_t index = m_stacks.size(); index > 0 && remaining > 0; --index)
    {
        ItemStack &stack = m_stacks[index - 1];
        if (stack.itemId != itemId)
            continue;

        const std::uint32_t removed = std::min(stack.quantity, remaining);
        stack.quantity -= removed;
        remaining -= removed;
        m_usedCapacity -= removed;
        if (stack.quantity == 0)
            m_stacks.erase(m_stacks.begin() + static_cast<std::ptrdiff_t>(index - 1));
    }
    return true;
}

void Inventory::clear()
{
    m_stacks.clear();
    m_usedCapacity = 0;
}

nlohmann::json Inventory::toJson() const
{
    nlohmann::json result;
    result["capacity"] = m_capacity;
    result["stacks"] = nlohmann::json::array();
    for (const ItemStack &stack : m_stacks)
        result["stacks"].push_back(itemStackToJson(stack));
    return result;
}

bool Inventory::fromJson(const nlohmann::json &value, const DefinitionResolver &resolver)
{
    if (!value.is_object() || !value.contains("stacks") ||
        !value.at("stacks").is_array() || !resolver)
        return false;

    std::uint32_t loadedCapacity = m_capacity;
    if (value.contains("capacity"))
    {
        if (!value.at("capacity").is_number_unsigned())
            return false;
        const std::uint64_t serializedCapacity = value.at("capacity").get<std::uint64_t>();
        if (serializedCapacity > kInventoryCapacity)
            return false;
        loadedCapacity = static_cast<std::uint32_t>(serializedCapacity);
    }

    Inventory loaded(loadedCapacity);
    for (const nlohmann::json &entry : value.at("stacks"))
    {
        ItemStack stack;
        if (!itemStackFromJson(entry, stack))
            return false;

        const ItemDefinition *definition = resolver(stack.itemId);
        if (!definition || definition->id != stack.itemId ||
            !loaded.add(*definition, stack.quantity))
            return false;
    }

    *this = std::move(loaded);
    return true;
}