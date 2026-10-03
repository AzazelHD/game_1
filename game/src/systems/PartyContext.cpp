#include "systems/PartyContext.h"

#include "data/SaveManager.h"
#include "inventory/Gear.h"
#include "inventory/GearCatalog.h"

namespace
{
    template <typename TGear, typename... TArgs>
    void addDebugGearToCatalogAndInventory(PartyContext &partyContext, ItemId itemId, const char *key,
                                           const std::string &displayName, const char *description,
                                           int goldValue,
                                           TArgs &&...args)
    {
        // The catalog is a persistent dictionary of definitions — it may
        // already hold this item from an earlier session/New Game. Ensure it
        // exists, then always place a copy in the (freshly cleared) inventory.
        if (partyContext.gearCatalog().find(itemId) == nullptr)
        {
            auto gear = std::make_unique<TGear>(ItemDefinition{
                                                    .id = itemId,
                                                    .key = key,
                                                    .displayName = displayName,
                                                    .description = description,
                                                    .goldValue = goldValue,
                                                },
                                                std::forward<TArgs>(args)...);

            if (!partyContext.gearCatalog().add(std::move(gear)))
                return;
        }

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
    m_gold = kStartingGold;

    // 1. Shipped starting kit (runs in both Release and Debug)
    seedDefaultStartingInventory();

#if defined(_DEBUG) || !defined(NDEBUG)
    // 2. Debug/test seed (auto-runs in Debug builds only)
    seedDebugInventoryWithTestGear();
#endif

    // 3. Fresh campaign: open the auto quest list so the spine is playable
    //    immediately; tavern-offered quests are picked up from the quest board.
    m_questLog.ensureLoaded();
    m_questLog.acceptAuto();

    m_initialized = true;
}

void PartyContext::resetForNewGame()
{
    // Start a second (or later) campaign in the same running session with a
    // fully fresh state, instead of inheriting the previous game's gold,
    // roster growth, party composition, and inventory. The gear catalog stays
    // (items are definitions the inventory references).
    m_gold = kStartingGold;

    m_roster.setUnits({});
    m_roster.ensureDefaultUnits();
    m_party.setMembers({});
    m_party.ensureDefaultFromRoster(m_roster);

    m_inventory.clear();
    seedDefaultStartingInventory();

#if defined(_DEBUG) || !defined(NDEBUG)
    seedDebugInventoryWithTestGear();
#endif

    m_questLog.ensureLoaded();
    m_questLog.acceptAuto();

    m_controlsTutorialShown = false;
    m_battleControlsTutorialShown = false;

    m_initialized = true;
}

bool PartyContext::applyCampaign(const CampaignSave &campaign)
{
    m_roster.setUnits(campaign.roster);
    m_gold = campaign.gold;

    if (campaign.partyMemberIds.empty())
        m_party.ensureDefaultFromRoster(m_roster);
    else
        m_party.setMembers(campaign.partyMemberIds);

    m_inventory.clear();

    Inventory loaded;
    if (!loaded.fromJson(campaign.inventoryJson, [this](ItemId itemId) -> const ItemDefinition *
                         { return m_gearCatalog.findItem(itemId); }))
        return false;
    m_inventory = std::move(loaded);

    // Restore quest progress. acceptance is rebuilt from the save; if the
    // save predates quests, open the catalog fresh (accept-all).
    if (campaign.questLogJson.is_object())
        m_questLog.fromJson(campaign.questLogJson);
    else
    {
        m_questLog.ensureLoaded();
        m_questLog.acceptAll();
    }

    m_initialized = true;
    return true;
}

std::vector<QuestDefinition> PartyContext::completeQuestsForNode(int nodeId, bool victorious)
{
    m_questLog.ensureLoaded();

    std::vector<QuestDefinition> completed;
    const std::vector<std::string> ids = m_questLog.completeByNode(nodeId, victorious);
    for (const std::string &id : ids)
    {
        const QuestDefinition *quest = m_questLog.find(id);
        if (!quest)
            continue;
        completed.push_back(*quest);

        // Gold
        if (quest->reward.gold > 0)
            addGold(quest->reward.gold);

        // Gear drops — one copy per declared key.
        for (const std::string &key : quest->reward.items)
        {
            const Gear *gear = m_gearCatalog.findByKey(key);
            if (!gear)
                continue;
            (void)m_inventory.add(*gear, 1);
        }
    }

    // Party XP — shared across the current party (mirrors battle victory).
    if (!completed.empty())
    {
        const int totalXp = [&completed]()
        {
            int sum = 0;
            for (const QuestDefinition &q : completed)
                sum += q.reward.xp;
            return sum;
        }();
        if (totalXp > 0)
        {
            for (int memberId : m_party.memberIds())
            {
                if (RosterUnit *unit = m_roster.findById(memberId))
                    unit->exp += totalXp;
            }
        }
    }

    return completed;
}

void PartyContext::seedDefaultStartingInventory()
{
    // Shipped placeholder starting kit for campaign start (Release & Debug)
    addDebugGearToCatalogAndInventory<Sword>(*this, 1, "starter_sword", "Starter Sword", "A reliable basic sword.", 120, GearStatModifiers{.attack = 3});
    addDebugGearToCatalogAndInventory<Shield>(*this, 2, "starter_shield", "Starter Shield", "A simple wooden shield.", 80, GearStatModifiers{.defense = 2});
    addDebugGearToCatalogAndInventory<Chest>(*this, 3, "starter_leather_armor", "Starter Leather Armor", "Light protective body armor.", 100, GearStatModifiers{.maxHp = 4, .defense = 1});
    addDebugGearToCatalogAndInventory<Shoes>(*this, 4, "starter_boots", "Starter Boots", "Simple travel boots.", 60, GearStatModifiers{.evasion = 1, .speed = 1});

    if (m_gearCatalog.find(5) == nullptr)
    {
        auto gear = std::make_unique<Amulet>(ItemDefinition{
                                                 .id = 5,
                                                 .key = "starter_ring",
                                                 .displayName = "Starter Copper Ring",
                                                 .description = "A simple copper ring.",
                                                 .goldValue = 50,
                                             },
                                             AccessorySubtype::Ring, GearStatModifiers{.magicDefense = 1}, GearSpecialEffect::None);
        (void)m_gearCatalog.add(std::move(gear));
    }
    if (const Gear *catalogGear = m_gearCatalog.find(5))
        (void)m_inventory.add(*catalogGear, 1);
}

void PartyContext::seedDebugInventoryWithTestGear()
{
    addDebugGearToCatalogAndInventory<Sword>(*this, 1001, "debug_sword_iron", "Debug Iron Sword", "Test item for equip flow validation.", 150, GearStatModifiers{.attack = 4, .speed = 1});
    addDebugGearToCatalogAndInventory<Mace>(*this, 1002, "debug_mace_steel", "Debug Steel Mace", "Test item for dual-wield in offhand.", 180, GearStatModifiers{.attack = 5, .defense = 1});
    addDebugGearToCatalogAndInventory<Shield>(*this, 1003, "debug_shield_wood", "Debug Wood Shield", "Test item for equip flow validation.", 90, GearStatModifiers{.defense = 3, .magicDefense = 1});
    addDebugGearToCatalogAndInventory<Helmet>(*this, 1004, "debug_helmet_steel", "Debug Steel Helmet", "Test item for equip flow validation.", 140, GearStatModifiers{.maxHp = 5, .defense = 2});
    addDebugGearToCatalogAndInventory<Chest>(*this, 1005, "debug_chest_leather", "Debug Leather Chest", "Test item for equip flow validation.", 130, GearStatModifiers{.maxHp = 8, .defense = 3});
    addDebugGearToCatalogAndInventory<Gloves>(*this, 1006, "debug_gloves_iron", "Debug Iron Gloves", "Test item for equip flow validation.", 70, GearStatModifiers{.attack = 1, .speed = 1});
    addDebugGearToCatalogAndInventory<Shoes>(*this, 1007, "debug_shoes_leather", "Debug Leather Shoes", "Test item for equip flow validation.", 75, GearStatModifiers{.defense = 1, .magicDefense = 0, .evasion = 2, .speed = 3});

    if (m_gearCatalog.find(1008) == nullptr)
    {
        auto gear = std::make_unique<Amulet>(ItemDefinition{
                                                 .id = 1008,
                                                 .key = "debug_amulet_rune",
                                                 .displayName = "Debug Rune Amulet",
                                                 .description = "Test item for equip flow validation.",
                                                 .goldValue = 300,
                                             },
                                             AccessorySubtype::Necklace, GearStatModifiers{.magic = 3, .magicDefense = 2}, GearSpecialEffect::None);
        (void)m_gearCatalog.add(std::move(gear));
    }
    if (const Gear *catalogGear = m_gearCatalog.find(1008))
        (void)m_inventory.add(*catalogGear, 1);

    addDebugGearToCatalogAndInventory<Bow>(*this, 1009, "debug_bow_oak", "Debug Oak Bow", "Two-handed ranged test weapon.", 160, GearStatModifiers{.attack = 4, .speed = 2});

    if (m_gearCatalog.find(1010) == nullptr)
    {
        auto gear = std::make_unique<Amulet>(ItemDefinition{
                                                 .id = 1010,
                                                 .key = "debug_ring_ruby",
                                                 .displayName = "Debug Ruby Ring",
                                                 .description = "Test accessory for slot validation.",
                                                 .goldValue = 320,
                                             },
                                             AccessorySubtype::Ring, GearStatModifiers{.maxHp = 6, .defense = 1}, GearSpecialEffect::None);
        (void)m_gearCatalog.add(std::move(gear));
    }
    if (const Gear *catalogGear = m_gearCatalog.find(1010))
        (void)m_inventory.add(*catalogGear, 1);
}
