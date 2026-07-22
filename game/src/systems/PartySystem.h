#pragma once
#include <vector>

class RosterSystem;

class PartySystem
{
public:
    void setMembers(std::vector<int> memberIds);
    void ensureDefaultFromRoster(const RosterSystem &roster, int maxMembers = 6);

    const std::vector<int> &memberIds() const { return m_memberIds; }

private:
    std::vector<int> m_memberIds;
};
