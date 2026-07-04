#include "systems/PartyContext.h"

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
    m_initialized = true;
}
