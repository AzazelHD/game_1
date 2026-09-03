#pragma once

#include "systems/PartySystem.h"
#include "systems/RosterSystem.h"
#include "inventory/GearCatalog.h"
#include "inventory/Inventory.h"

class PartyContext
{
public:
    static PartyContext &instance();

    void ensureInitialized();

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

private:
    bool m_initialized = false;
    RosterSystem m_roster;
    PartySystem m_party;
    Inventory m_inventory;
    GearCatalog m_gearCatalog;
};
