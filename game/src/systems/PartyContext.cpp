#include "systems/PartyContext.h"

#include "inventory/Gear.h"
#include "inventory/GearCatalog.h"

namespace
{
    template <typename TGear, typename... TArgs>
    void addDebugGearToCatalogAndInventory(PartyContext &partyContext, ItemId itemId, const char *key,
                                           const std::string &displayName, const char *description,
                                           TArgs &&...args)
    {
        if (partyContext.gearCatalog().find(itemId) != nullptr)
            return;

        auto gear = std::make_unique<TGear>(ItemDefinition{
                                                .id = itemId,
                                                .key = key,
                                                .displayName = displayName,
                                                .description = description,
                                                .stackable = false,
                                                .maxStackSize = 1,
                                            },
                                            std::forward<TArgs>(args)...);

        if (!partyContext.gearCatalog().add(std::move(gear)))
            return;

        const Gear *catalogGear = partyContext.gearCatalog().find(itemId);
        if (catalogGear)
            (void)partyContext.inventory().add(*catalogGear, 1);
    }
}

PartyContext &PartyContext::instance()
{
    static PartyContext ctx;
    return ctx;
}

void PartyContext::ensureInitialized()
{
    if (m_initialized)
        return;

    m_roster.ensureDefaultUnits();
    m_party.ensureDefaultFromRoster(m_roster);

    // 1. Shipped starting kit (runs in both Release and Debug)
    seedDefaultStartingInventory();

#if defined(_DEBUG) || !defined(NDEBUG)
    // 2. Debug/test seed (auto-runs in Debug builds only)
    seedDebugInventoryWithTestGear();
#endif

    m_initialized = true;
}

void PartyContext::seedDefaultStartingInventory()
{
    // Shipped placeholder starting kit for campaign start (Release & Debug)
    addDebugGearToCatalogAndInventory<Sword>(*this, 1, "starter_sword", "Starter Sword", "A reliable basic sword.", GearStatModifiers{.attack = 3});
    addDebugGearToCatalogAndInventory<Shield>(*this, 2, "starter_shield", "Starter Shield", "A simple wooden shield.", GearStatModifiers{.defense = 2});
    addDebugGearToCatalogAndInventory<Chest>(*this, 3, "starter_leather_armor", "Starter Leather Armor", "Light protective body armor.", GearStatModifiers{.maxHp = 4, .defense = 1});
    addDebugGearToCatalogAndInventory<Shoes>(*this, 4, "starter_boots", "Starter Boots", "Simple travel boots.", GearStatModifiers{.evasion = 1, .speed = 1});

    if (m_gearCatalog.find(5) == nullptr)
    {
        auto gear = std::make_unique<Amulet>(ItemDefinition{
                                                 .id = 5,
                                                 .key = "starter_ring",
                                                 .displayName = "Starter Copper Ring",
                                                 .description = "A simple copper ring.",
                                                 .stackable = false,
                                                 .maxStackSize = 1,
                                             },
                                             AccessorySubtype::Ring, GearStatModifiers{.magicDefense = 1}, GearSpecialEffect::None);
        if (m_gearCatalog.add(std::move(gear)))
            (void)m_inventory.add(*m_gearCatalog.find(5), 1);
    }
}

void PartyContext::seedDebugInventoryWithTestGear()
{
    addDebugGearToCatalogAndInventory<Sword>(*this, 1001, "debug_sword_iron", "Debug Iron Sword", "Test item for equip flow validation.", GearStatModifiers{.attack = 4, .speed = 1});
    addDebugGearToCatalogAndInventory<Mace>(*this, 1002, "debug_mace_steel", "Debug Steel Mace", "Test item for dual-wield in offhand.", GearStatModifiers{.attack = 5, .defense = 1});
    addDebugGearToCatalogAndInventory<Shield>(*this, 1003, "debug_shield_wood", "Debug Wood Shield", "Test item for equip flow validation.", GearStatModifiers{.defense = 3, .magicDefense = 1});
    addDebugGearToCatalogAndInventory<Helmet>(*this, 1004, "debug_helmet_steel", "Debug Steel Helmet", "Test item for equip flow validation.", GearStatModifiers{.maxHp = 5, .defense = 2});
    addDebugGearToCatalogAndInventory<Chest>(*this, 1005, "debug_chest_leather", "Debug Leather Chest", "Test item for equip flow validation.", GearStatModifiers{.maxHp = 8, .defense = 3});
    addDebugGearToCatalogAndInventory<Gloves>(*this, 1006, "debug_gloves_iron", "Debug Iron Gloves", "Test item for equip flow validation.", GearStatModifiers{.attack = 1, .speed = 1});
    addDebugGearToCatalogAndInventory<Shoes>(*this, 1007, "debug_shoes_leather", "Debug Leather Shoes", "Test item for equip flow validation.", GearStatModifiers{.defense = 1, .magicDefense = 0, .evasion = 2, .speed = 3});

    if (m_gearCatalog.find(1008) == nullptr)
    {
        auto gear = std::make_unique<Amulet>(ItemDefinition{
                                                 .id = 1008,
                                                 .key = "debug_amulet_rune",
                                                 .displayName = "Debug Rune Amulet",
                                                 .description = "Test item for equip flow validation.",
                                                 .stackable = false,
                                                 .maxStackSize = 1,
                                             },
                                             AccessorySubtype::Necklace, GearStatModifiers{.magic = 3, .magicDefense = 2}, GearSpecialEffect::None);
        if (m_gearCatalog.add(std::move(gear)))
            (void)m_inventory.add(*m_gearCatalog.find(1008), 1);
    }

    addDebugGearToCatalogAndInventory<Bow>(*this, 1009, "debug_bow_oak", "Debug Oak Bow", "Two-handed ranged test weapon.", GearStatModifiers{.attack = 4, .speed = 2});

    if (m_gearCatalog.find(1010) == nullptr)
    {
        auto gear = std::make_unique<Amulet>(ItemDefinition{
                                                 .id = 1010,
                                                 .key = "debug_ring_ruby",
                                                 .displayName = "Debug Ruby Ring",
                                                 .description = "Test accessory for slot validation.",
                                                 .stackable = false,
                                                 .maxStackSize = 1,
                                             },
                                             AccessorySubtype::Ring, GearStatModifiers{.maxHp = 6, .defense = 1}, GearSpecialEffect::None);
        if (m_gearCatalog.add(std::move(gear)))
            (void)m_inventory.add(*m_gearCatalog.find(1010), 1);
    }
}
