#pragma once

#include <string>
#include <vector>

class RosterSystem;

class PartySystem
{
public:
    void setMembers(std::vector<std::string> memberIds);
    void ensureDefaultFromRoster(const RosterSystem &roster, int maxMembers = 6);

    const std::vector<std::string> &memberIds() const { return m_memberIds; }

private:
    std::vector<std::string> m_memberIds;
};
