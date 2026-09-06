#include "battle/combat/PendingAttackController.h"

#include <algorithm>

#include "battle/unit/Unit.h"
#include "battle/map/Grid.h"
#include "battle/map/BattleMap.h"
#include "battle/map/AttackRange.h"
#include "data/SkillLoader.h" // SkillData

void PendingAttackController::begin(Unit *active, Vec2i targetPos, Unit *directTarget,
                                    const SkillData *skill, const RangeRule &rule,
                                    const Grid &grid, const BattleMap &battleMap,
                                    const std::vector<Unit *> &units)
{
    m_targets.clear();
    m_tiles.clear();
    m_center = targetPos;
    m_focus = 0;

    if (!active)
        return;

    if (skill && skill->area > 0)
    {
        // AoE hits everyone in range regardless of team — including the
        // caster themself if they're standing in the blast. Only
        // single-target (area == 0) is restricted to enemies, and that
        // restriction lives in HumanTurnController's targeting instead.
        const std::unordered_set<Vec2i, Vec2iHash> splashTiles =
            AttackRange::computeSplashTiles(grid, battleMap, targetPos, rule);
        for (Unit *u : units)
        {
            if (!u || u->isDead())
                continue;
            if (splashTiles.count(u->getPosition()))
                m_targets.push_back(u);
        }
    }
    else if (directTarget)
    {
        m_targets.push_back(directTarget);
    }

    for (Unit *u : m_targets)
        m_tiles.insert(u->getPosition());
}

void PendingAttackController::cycleFocus(int delta)
{
    if (m_targets.empty())
        return;
    const int count = static_cast<int>(m_targets.size());
    m_focus = (m_focus + delta + count) % count;
}

void PendingAttackController::clear()
{
    m_targets.clear();
    m_tiles.clear();
    m_center = Vec2i{0, 0};
    m_focus = 0;
}

Unit *PendingAttackController::focusedTarget()
{
    if (m_targets.empty())
        return nullptr;
    const int idx = std::clamp(m_focus, 0, static_cast<int>(m_targets.size()) - 1);
    return m_targets[idx];
}
