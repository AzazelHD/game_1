#pragma once
#include <vector>

class Unit;

// A single entry in the upcoming-turns preview.
struct TurnEntry
{
    Unit *unit = nullptr;
    float timeUntilTurn = 0.f; // lower = sooner
};

// TurnQueue manages a continuous timeline of upcoming unit turns.
// There are no rounds — after each unit acts it is re-inserted into
// the timeline based on its speed and what actions it took.
//
// Dead units stay in the timeline at full cost. This supports
// auto-revive mechanics (e.g. a zombie that revives after N turns):
// BattleState checks whether the unit should revive when it surfaces
// at the front of the timeline via getCurrentUnit().
//
// The preview is only ever rebuilt inside advance(), so speed buffs
// or debuffs applied mid-turn are picked up naturally on the next advance().
//
// Cost constants are public so BattleState can compute the float to pass in:
//   float cost = 0.f;
//   if (unit->hasMoved())  cost += TurnQueue::BASE_MOVE_COST;
//   if (unit->hasActed())  cost += TurnQueue::BASE_ACTION_COST;
//   if (cost == 0.f)       cost  = TurnQueue::BASE_WAIT_COST;
//   cost /= unit->getSpeed();
//   queue.advance(cost);
//
// Typical call sequence each turn:
//   1. getCurrentUnit()    — who acts now (may be dead; caller handles revive/skip)
//   2. ... unit takes actions ...
//   3. advance(cost)       — re-insert unit, rebuild preview
//   4. getCurrentUnit()    — next unit, repeat
//
// Design note: TurnQueue holds non-owning Unit* pointers.
//              Actual Unit objects are owned by BattleState.
class TurnQueue
{
public:
    static constexpr float BASE_WAIT_COST = 250.f;
    static constexpr float BASE_MOVE_COST = 500.f;
    static constexpr float BASE_ACTION_COST = 750.f;

    // Populate the timeline from a list of units and build the initial preview.
    // Units are inserted with timeUntilTurn = (BASE_MOVE_COST + BASE_ACTION_COST) / speed,
    // offset slightly so equal-speed units have a deterministic order.
    void init(std::vector<Unit *> units);

    // The unit whose turn it is right now (front of the timeline).
    // May return a dead unit — caller is responsible for revive/skip logic.
    Unit *getCurrentUnit() const;

    // Call after the current unit finishes acting.
    // Pops the front unit, re-inserts it at timeUntilTurn = front.timeUntilTurn + timeCost,
    // normalises the timeline so the front is always at 0, then rebuilds the preview.
    void advance(float timeCost);

    // Removes a unit from the timeline and preview entirely.
    // Call when a unit disappears on death (no revive possible).
    // Safe to call mid-battle — rebuilds preview automatically.
    void remove(Unit *unit);

    // TurnQueue.h
    // Inserts a new unit into the timeline at the given time offset from now.
    // Used when a unit is spawned or replaced mid-battle.
    void insert(Unit *unit, float timeOffset);

    // Read-only view of the next PREVIEW_SIZE upcoming turns.
    // Index 0 is always the current unit (same as getCurrentUnit()).
    // Only valid to read between turns, never mid-turn.
    const std::vector<TurnEntry> &getPreview() const;

private:
    // Simulate the next PREVIEW_SIZE turns from the current timeline
    // state and store the result in m_preview. Does not mutate m_timeline.
    void rebuildPreview();

    static constexpr int PREVIEW_SIZE = 20;

    std::vector<TurnEntry> m_timeline; // sorted ascending by timeUntilTurn, front is always 0
    std::vector<TurnEntry> m_preview;  // next PREVIEW_SIZE entries, rebuilt each advance()
};