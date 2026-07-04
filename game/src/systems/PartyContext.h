#pragma once

#include "systems/PartySystem.h"
#include "systems/RosterSystem.h"

class PartyContext
{
public:
    static PartyContext &instance();

    void ensureInitialized();

    RosterSystem &roster() { return m_roster; }
    PartySystem &party() { return m_party; }

private:
    bool m_initialized = false;
    RosterSystem m_roster;
    PartySystem m_party;
};
