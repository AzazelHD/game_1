#include "battle/combat/TurnQueue.h"
#include "battle/unit/Unit.h"

#include <algorithm>
#include <cassert>

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

static void sortTimeline(std::vector<TurnEntry> &timeline)
{
    std::sort(timeline.begin(), timeline.end(),
              [](const TurnEntry &a, const TurnEntry &b)
              {
                  return a.timeUntilTurn < b.timeUntilTurn;
              });
}

// Shift all entries so the front is at 0.
// This keeps timeUntilTurn values small and avoids float drift over long battles.
static void normalise(std::vector<TurnEntry> &timeline)
{
    if (timeline.empty())
        return;
    const float offset = timeline.front().timeUntilTurn;
    if (offset == 0.f)
        return;
    for (auto &e : timeline)
        e.timeUntilTurn -= offset;
}

// ---------------------------------------------------------------------------
// public
// ---------------------------------------------------------------------------

float TurnQueue::scaledCost(float baseCost, int speed)
{
    assert(speed > 0);
    return baseCost / static_cast<float>(speed);
}

float TurnQueue::turnCostFor(const Unit &unit)
{
    float baseCost;
    if (unit.hasMoved() && unit.hasActed())
        baseCost = BASE_MOVE_AND_ACTION_COST;
    else if (unit.hasActed())
        baseCost = BASE_ACTION_COST;
    else if (unit.hasMoved())
        baseCost = BASE_MOVE_COST;
    else
        baseCost = BASE_WAIT_COST;

    return scaledCost(baseCost, unit.getSpeed());
}

void TurnQueue::init(std::vector<Unit *> units)
{
    m_timeline.clear();
    m_preview.clear();

    // Insert every unit with a full-turn cost as their initial delay.
    // A small index-based offset breaks ties between equal-speed units
    // deterministically (first in the list goes first).
    for (std::size_t i = 0; i < units.size(); ++i)
    {
        Unit *u = units[i];
        float time = scaledCost(BASE_MOVE_AND_ACTION_COST, u->getSpeed()) +
                     static_cast<float>(i) * 0.001f;

        m_timeline.push_back({u, time});
    }

    sortTimeline(m_timeline);
    normalise(m_timeline);
    rebuildPreview();
}

void TurnQueue::moveToFront(Unit *unit)
{
    auto it = std::find_if(m_timeline.begin(), m_timeline.end(),
                           [unit](const TurnEntry &e) { return e.unit == unit; });
    if (it == m_timeline.end() || it == m_timeline.begin())
        return;

    // Lift the unit out and re-insert it at the head. Every other unit keeps its
    // own time and its relative order — they just slide down one slot. Swapping
    // two units' times instead would leave them where they were, reordering them.
    TurnEntry moved = *it;
    m_timeline.erase(it);

    // Place it a hair ahead of whoever is currently first. Keeping its own time
    // would be wrong: the unit being moved is normally *later* than the front,
    // and normalise() shifts by the front's value, so the untouched units would
    // land on negative times — i.e. everyone else would get a free head start.
    const float firstTime = m_timeline.empty() ? 0.f : m_timeline.front().timeUntilTurn;
    moved.timeUntilTurn = firstTime - 1.f;
    m_timeline.insert(m_timeline.begin(), moved);

    // Deliberately no sortTimeline(): the point is to override the time order.
    normalise(m_timeline);
    rebuildPreview();
}

Unit *TurnQueue::getCurrentUnit() const
{
    // Empty timeline is a legitimate state: the queue exists from
    // BattleSession::init({}) until startCombatPhase() seeds it (and after
    // every unit is removed). Every caller already handles nullptr, so this
    // is a normal answer, not a programming error.
    if (m_timeline.empty())
        return nullptr;
    return m_timeline.front().unit;
}

void TurnQueue::advance(float timeCost)
{
    assert(!m_timeline.empty());
    assert(timeCost > 0.f);

    // Re-insert the current unit at the back of time.
    // Its new absolute time = 0 (current front) + timeCost,
    // but after normalise() the front will be 0 again anyway,
    // so we just store timeCost as the raw value before normalising.
    TurnEntry acted = m_timeline.front();
    acted.timeUntilTurn = timeCost; // relative to current time (which is 0)

    m_timeline.erase(m_timeline.begin()); // pop front
    m_timeline.push_back(acted);          // push to back (unsorted position)

    sortTimeline(m_timeline); // re-sort
    normalise(m_timeline);    // keep front at 0
    rebuildPreview();
}

const std::vector<TurnEntry> &TurnQueue::getPreview() const
{
    return m_preview;
}

// ---------------------------------------------------------------------------
// private
// ---------------------------------------------------------------------------

void TurnQueue::rebuildPreview()
{
    m_preview.clear();
    m_preview.reserve(PREVIEW_SIZE);

    // Work on a local copy so we don't mutate m_timeline.
    std::vector<TurnEntry> sim = m_timeline;

    while (static_cast<int>(m_preview.size()) < PREVIEW_SIZE)
    {
        if (sim.empty())
            break;

        // Front of the sim is the next unit to act.
        m_preview.push_back(sim.front());

        // Re-insert it with a full-cost delay (worst case assumption,
        // since we don't know what it will do in the future). This must match
        // init()'s opening cost, or the preview's spacing drifts away from the
        // real timeline's.
        TurnEntry next = sim.front();
        next.timeUntilTurn = scaledCost(BASE_MOVE_AND_ACTION_COST, next.unit->getSpeed());

        sim.erase(sim.begin());
        sim.push_back(next);
        sortTimeline(sim);
        normalise(sim);
    }
}

void TurnQueue::insert(Unit *unit, float timeOffset)
{
    m_timeline.push_back({unit, timeOffset});
    sortTimeline(m_timeline);
    normalise(m_timeline);
    rebuildPreview();
}

void TurnQueue::remove(Unit *unit)
{
    m_timeline.erase(
        std::remove_if(m_timeline.begin(), m_timeline.end(),
                       [unit](const TurnEntry &e)
                       { return e.unit == unit; }),
        m_timeline.end());

    normalise(m_timeline);
    rebuildPreview();
}
