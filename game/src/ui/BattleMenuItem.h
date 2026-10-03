#pragma once

#include <string>
#include <functional>

struct BattleMenuItem
{
    std::string label;
    bool enabled = true;
    std::function<void()> onSelect;
    // Longer text revealed with Details (Tab) while this row is highlighted.
    // Empty = the row has nothing to explain. Skill rows use the skill's
    // authored description.
    std::string detail;
};
