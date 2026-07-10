#pragma once
#include <vector>
#include <string>
#include "battle/Unit.h"
#include "battle/TurnQueue.h"
#include "battle/CombatSystem.h"

enum class BattleResult
{
    Ongoing,
    Victory,
    Defeat,
};

// Describes one unit's spawn — loaded from the battle JSON.
struct UnitSpawn
{
    std::string unitFilePath; // path to unit JSON
    Vec2i startPos;
    int team = 0;
    bool isCritical = false;       // defeat if this unit dies
    bool isBoss = false;           // victory if this unit dies
    bool disappearOnDeath = false; // if true, slot is freed and can be reused
};

// BattleSession owns all runtime data for one battle:
// units, turn queue, and battle result.
//
// BattleState owns a BattleSession and delegates all game logic to it.
// BattleState itself handles rendering and input only.
//
// Lifecycle:
//   init(spawns)  — load units from JSON, build TurnQueue, mark critical units
//   update(dt)    — advance turn logic (called by BattleState::update)
//   getResult()   — check if battle is over
//
// Mid-battle spawns (reinforcements, summons):
//   spawnUnit(spawn) — safe as long as m_units.size() < m_units.capacity().
//   Always call init() with a large enough reserve (default: 64 slots).
//   An assert fires in debug if capacity is exceeded before a reallocation
//   can invalidate TurnQueue's non-owning Unit* pointers.
class BattleSession
{
public:
    // Load all units from spawn list and initialise the turn queue.
    // Reserves UNIT_CAPACITY slots upfront so mid-battle spawns are safe.
    // TurnQueue holds non-owning Unit* pointers — m_units must never reallocate.
    void init(const std::vector<UnitSpawn> &spawns);

    // Constructs a Unit from a spawn without adding it to m_units.
    // Used by replaceUnit() to overwrite an existing slot in place.
    Unit makeUnit(const UnitSpawn &spawn);

    // Spawn a new unit mid-battle (reinforcement, summon, etc.).
    // Inserts the unit into m_units and re-inserts it into the turn queue.
    // Asserts that m_units.size() < m_units.capacity() to prevent reallocation.
    void spawnUnit(const UnitSpawn &spawn);

    // Replaces a disappeared unit's slot with a new spawn.
    // Used for wave missions — reuses the same memory slot so
    // TurnQueue pointers stay valid.
    void replaceUnit(Unit *dead, const UnitSpawn &spawn);

    // Called every frame by BattleState. Drives turn logic.
    void update(float dt);

    // Current battle result.
    BattleResult getResult() const { return m_result; }

    // Read-only access for rendering.
    const std::vector<Unit> &getUnits() const { return m_units; }
    // Non-owning pointer view for consumers that need Unit* (HumanTurnController,
    // BattleRendererContext) — valid as long as m_units never reallocates
    // (guaranteed by the UNIT_CAPACITY reserve).
    std::vector<Unit *> &getUnitPtrs() { return m_unitPtrs; }
    const std::vector<Unit *> &getUnitPtrs() const { return m_unitPtrs; }
    const std::vector<TurnEntry> &getPreview() const { return m_queue.getPreview(); }
    Unit *getCurrentUnit() { return m_queue.getCurrentUnit(); }
    Unit *getCurrentUnit() const { return m_queue.getCurrentUnit(); }
    // Pass-through for callers that need to advance the turn without going
    // through applyResult() (e.g. skipping a dead unit's turn).
    void advanceTurn(float timeCost) { m_queue.advance(timeCost); }

    // Applies a resolved hit's damage/heal to `target`. Does NOT advance the
    // turn queue — this game allows multiple actions/targets (e.g. an AoE
    // hitting several units) within a single turn, with turn advancement
    // happening later as its own explicit step (advanceTurn(), already used
    // by BattleState::advanceToNextUnit()). Bundling both into one call, as
    // the old applyResult() did, would advance the queue once per target hit.
    void applyDamage(Unit &target, const CombatResult &result);

    // Re-checks victory/defeat conditions. Call after applyDamage() (or any
    // other state change that could end the battle).
    void checkResult() { checkBattleResult(); }

private:
    static constexpr int UNIT_CAPACITY = 16; // max units per battle incl. reinforcements

    std::vector<Unit> m_units;      // owns all units — never reallocates after init()
    std::vector<Unit *> m_unitPtrs; // parallel pointer view, kept in sync in init()/spawnUnit()
    TurnQueue m_queue;
    BattleResult m_result = BattleResult::Ongoing;

    // Non-owning pointers into m_units — safe because m_units never reallocates.
    std::vector<Unit *> m_criticalUnits; // defeat immediately if any of these die
    std::vector<Unit *> m_bossUnits;     // victory immediately if any of these die

    // Shared unit construction logic used by both init() and spawnUnit().
    Unit &buildUnit(const UnitSpawn &spawn);

    // Win/lose condition checks — called inside applyResult().
    void checkBattleResult();

    // True if all units on a given team are dead.
    bool allTeamDead(int team) const;
};