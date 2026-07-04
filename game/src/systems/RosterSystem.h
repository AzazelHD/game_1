#pragma once

#include <string>
#include <vector>

struct RosterUnit
{
    std::string unitId;
    std::string templatePath;
    bool recruited = true;
};

class RosterSystem
{
public:
    void setUnits(std::vector<RosterUnit> units);
    void ensureDefaultUnits();

    const std::vector<RosterUnit> &units() const { return m_units; }
    const RosterUnit *findById(const std::string &unitId) const;

private:
    std::vector<RosterUnit> m_units;
};
