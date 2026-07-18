#include "systems/RosterSystem.h"

#include <algorithm>

void RosterSystem::setUnits(std::vector<RosterUnit> units)
{
    m_units = std::move(units);

    // Keep the next-id counter ahead of whatever was loaded (e.g. from a
    // save file), so recruit() never hands out an id that's already in use.
    for (const RosterUnit &u : m_units)
        m_nextInstanceId = std::max(m_nextInstanceId, u.instanceId + 1);
}

void RosterSystem::ensureDefaultUnits()
{
    if (!m_units.empty())
        return;

    recruit("assets/units/aria.json", "Aria");
    recruit("assets/units/soldier.json", "Soldier");
}

RosterUnit *RosterSystem::recruit(std::string templatePath, std::string customName)
{
    m_units.push_back(RosterUnit{
        .instanceId = m_nextInstanceId++,
        .templatePath = std::move(templatePath),
        .customName = std::move(customName),
        .recruited = true,
    });
    return &m_units.back();
}

void RosterSystem::dismiss(int instanceId)
{
    m_units.erase(
        std::remove_if(m_units.begin(), m_units.end(),
                       [instanceId](const RosterUnit &u)
                       { return u.instanceId == instanceId; }),
        m_units.end());
}

const RosterUnit *RosterSystem::findById(int instanceId) const
{
    for (const RosterUnit &u : m_units)
    {
        if (u.instanceId == instanceId)
            return &u;
    }
    return nullptr;
}

const RosterUnit *RosterSystem::findByRef(const std::string &unitRef) const
{
    for (const RosterUnit &u : m_units)
    {
        if (!u.customName.empty() && u.customName == unitRef)
            return &u;
    }
    for (const RosterUnit &u : m_units)
    {
        if (u.templatePath == unitRef)
            return &u;
    }
    return nullptr;
}