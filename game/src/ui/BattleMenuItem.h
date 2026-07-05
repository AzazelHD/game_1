#pragma once

#include <string>
#include <functional>

struct BattleMenuItem
{
    std::string label;
    bool enabled = true;
    std::function<void()> onSelect;
};