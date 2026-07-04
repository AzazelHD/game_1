#include "systems/RosterSystem.h"

void RosterSystem::setUnits(std::vector<RosterUnit> units)
{
    m_units = std::move(units);
}

void RosterSystem::ensureDefaultUnits()
{
    if (!m_units.empty())
        return;

    m_units.push_back(RosterUnit{
        .unitId = "soldier",
        .templatePath = "assets/units/soldier.json",
        .recruited = true,
    });

    m_units.push_back(RosterUnit{
        .unitId = "aria",
        .templatePath = "assets/units/aria.json",
        .recruited = true,
    });
}

const RosterUnit *RosterSystem::findById(const std::string &unitId) const
{
    for (const RosterUnit &u : m_units)
    {
        if (u.unitId == unitId)
            return &u;
    }
    return nullptr;
}
