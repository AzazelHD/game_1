#include "systems/PartySystem.h"
#include "systems/RosterSystem.h"

void PartySystem::setMembers(std::vector<int> memberIds)
{
    m_memberIds = std::move(memberIds);
}

void PartySystem::ensureDefaultFromRoster(const RosterSystem &roster, int maxMembers)
{
    if (!m_memberIds.empty())
        return;

    for (const RosterUnit &u : roster.units())
    {
        if (!u.recruited)
            continue;

        m_memberIds.push_back(u.instanceId);
        if (static_cast<int>(m_memberIds.size()) >= maxMembers)
            break;
    }
}
