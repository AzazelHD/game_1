#pragma once

#include "systems/PartySystem.h"
#include "systems/RosterSystem.h"
#include "systems/QuestLog.h"
#include "inventory/GearCatalog.h"
#include "inventory/Inventory.h"

#include <string>

struct CampaignSave;

// Starting purse for a fresh campaign (New Game). Earned gold comes from
// battle rewards; this only seeds the initial kit.
inline constexpr int kStartingGold = 300;

class PartyContext
{
public:
    static PartyContext &instance();

    void ensureInitialized();

    // Fully resets the singleton back to fresh-campaign state (gold, roster,
    // party, inventory) so a New Game started later in the same session does
    // not inherit earlier progress. The gear catalog dictionary persists.
    void resetForNewGame();

    // Replaces roster + party + inventory contents with a loaded campaign.
    // Assumes ensureInitialized() has run (so the gear catalog exists to
    // resolve inventory ItemIds).
    bool applyCampaign(const CampaignSave &campaign);

    // Shipped campaign starting kit (placeholder items for new game; runs in Release and Debug)
    void seedDefaultStartingInventory();

    // DEBUG/TEST ONLY: explicit test gear seeding for equipment flow validation.
    // Auto-runs in Debug builds only; never ships in Release.
    void seedDebugInventoryWithTestGear();

    RosterSystem &roster() { return m_roster; }
    PartySystem &party() { return m_party; }
    Inventory &inventory() { return m_inventory; }
    const Inventory &inventory() const { return m_inventory; }
    GearCatalog &gearCatalog() { return m_gearCatalog; }
    const GearCatalog &gearCatalog() const { return m_gearCatalog; }
    QuestLog &questLog() { return m_questLog; }
    const QuestLog &questLog() const { return m_questLog; }

    int gold() const { return m_gold; }
    void setGold(int gold) { m_gold = gold; }
    int addGold(int amount) { m_gold += amount; return m_gold; }
    bool spendGold(int amount)
    {
        if (amount < 0 || m_gold < amount)
            return false;
        m_gold -= amount;
        return true;
    }

    // Completes any active quest gated on nodeId (victory or arrival) and
    // applies its rewards (gold, gear drops, party XP) to this campaign.
    // Returns the newly-completed quest definitions (for journal/toast UI).
    std::vector<QuestDefinition> completeQuestsForNode(int nodeId, bool victorious);

    // First-time controls tutorial: shown once when the player picks up their
    // first tavern quest. Fresh games start un-shown; a load restores whatever
    // the campaign had (see restoreTutorialFlags), so tips never replay.
    bool controlsTutorialShown() const { return m_controlsTutorialShown; }
    void markControlsTutorialShown() { m_controlsTutorialShown = true; }

    // First-time battle controls tutorial: shown once at the start of the
    // first combat, same lifetime rules as above.
    bool battleControlsTutorialShown() const { return m_battleControlsTutorialShown; }
    void markBattleControlsTutorialShown() { m_battleControlsTutorialShown = true; }

    // Restores both first-time-tip flags from a campaign save. Set, not OR:
    // loading must not inherit flags from a campaign played earlier in the
    // session.
    void restoreTutorialFlags(bool controlsShown, bool battleShown)
    {
        m_controlsTutorialShown = controlsShown;
        m_battleControlsTutorialShown = battleShown;
    }

private:
    bool m_initialized = false;
    bool m_controlsTutorialShown = false;
    bool m_battleControlsTutorialShown = false;
    int m_gold = 0;
    RosterSystem m_roster;
    PartySystem m_party;
    Inventory m_inventory;
    GearCatalog m_gearCatalog;
    QuestLog m_questLog;
};
