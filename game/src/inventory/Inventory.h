#pragma once

#include "inventory/Item.h"

#include <cstddef>
#include <functional>
#include <vector>

// Capacity is measured in individual item units, not distinct IDs or stack
// entries. It is intentionally a power-of-two placeholder for later tuning.
inline constexpr std::uint32_t kInventoryCapacity = 65536;

class Inventory
{
public:
    using DefinitionResolver = std::function<const ItemDefinition *(ItemId)>;

    explicit Inventory(std::uint32_t capacity = kInventoryCapacity);

    std::uint32_t capacity() const { return m_capacity; }
    std::uint32_t usedCapacity() const { return m_usedCapacity; }
    std::uint32_t remainingCapacity() const { return m_capacity - m_usedCapacity; }
    std::size_t stackCount() const { return m_stacks.size(); }

    bool contains(ItemId itemId) const;
    std::uint32_t quantity(ItemId itemId) const;
    bool has(ItemId itemId, std::uint32_t requestedQuantity) const;

    // Adds atomically: if the full quantity cannot fit, no quantity is added.
    // Stackable items may create multiple entries when a stack reaches its cap.
    bool add(const ItemDefinition &definition, std::uint32_t quantity);
    bool remove(ItemId itemId, std::uint32_t quantity);

    void clear();
    const std::vector<ItemStack> &stacks() const { return m_stacks; }

    nlohmann::json toJson() const;
    bool fromJson(const nlohmann::json &value, const DefinitionResolver &resolver);

private:
    bool canAdd(const ItemDefinition &definition, std::uint32_t quantity) const;

    std::uint32_t m_capacity;
    std::uint32_t m_usedCapacity = 0;
    std::vector<ItemStack> m_stacks;
};