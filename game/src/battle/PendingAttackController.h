#pragma once

#include "engine/math/Vec2.h"
#include "battle/MovementRange.h"

#include <vector>
#include <unordered_set>

class Unit;
struct SkillData;

class PendingAttackController
{
public:
    // Builds the target/tile set from the skill area (or a single direct target).
    void begin(Unit *active, Vec2i targetPos, Unit *directTarget,
               const SkillData *skill, const std::vector<Unit *> &units);

    void cycleFocus(int delta);
    void clear();

    bool empty() const { return m_targets.empty(); }
    Vec2i center() const { return m_center; }
    Unit *focusedTarget();
    const std::vector<Unit *> &targets() const { return m_targets; }
    const std::unordered_set<Vec2i, Vec2iHash> &tiles() const { return m_tiles; }

private:
    std::vector<Unit *> m_targets;
    std::unordered_set<Vec2i, Vec2iHash> m_tiles;
    Vec2i m_center{0, 0};
    int m_focus = 0;
};
