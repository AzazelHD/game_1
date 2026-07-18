#pragma once
#include <string>
#include <vector>

struct RosterUnit
{
    int instanceId = -1;      // assigned once at recruitment, never reused or changed — the real unique key
    std::string templatePath; // which class/job this is — e.g. "assets/units/soldier.json"
    std::string customName;   // empty = use the template's default name; set for named uniques (Marche, Montblanc) or renamed generics
    bool recruited = true;
};

class RosterSystem
{
public:
    void setUnits(std::vector<RosterUnit> units);
    void ensureDefaultUnits();

    // Adds a new unit with a fresh, never-reused instanceId. Returns a
    // pointer into m_units (valid until the next structural change to the
    // vector — same caveat as any vector-of-value API).
    RosterUnit *recruit(std::string templatePath, std::string customName = "");

    // Removes the unit with this instanceId, if present. The vector
    // reorders/shrinks freely — instanceId is never reassigned to anyone
    // else, so nothing else needs to change when this happens.
    void dismiss(int instanceId);

    const std::vector<RosterUnit> &units() const { return m_units; }
    const RosterUnit *findById(int instanceId) const;
    const RosterUnit *findByRef(const std::string &unitRef) const;

private:
    std::vector<RosterUnit> m_units;
    int m_nextInstanceId = 0; // monotonically increasing, never decremented or reused
};