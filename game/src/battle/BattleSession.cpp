#include "battle/BattleSession.h"
#include "data/UnitLoader.h"
#include "battle/UnitProgression.h"
#include <algorithm>
#include <cassert>

// ---------------------------------------------------------------------------
// public
// ---------------------------------------------------------------------------

void BattleSession::init(const std::vector<UnitSpawn> &spawns,
                         BattleVictoryRule victoryRule,
                         BattleDefeatRule defeatRule)
{
    m_units.clear();
    m_units.reserve(UNIT_CAPACITY);
    m_unitPtrs.clear();
    m_unitPtrs.reserve(UNIT_CAPACITY);
    m_criticalUnits.clear();
    m_bossUnits.clear();
    m_result = BattleResult::Ongoing;
    m_victoryRule = victoryRule;
    m_defeatRule = defeatRule;
    m_turnsElapsed = 0;

    for (const auto &spawn : spawns)
    {
        Unit &unit = buildUnit(spawn);
        m_unitPtrs.push_back(&unit);

        if (spawn.isCritical)
            m_criticalUnits.push_back(&unit);
        if (spawn.isBoss)
            m_bossUnits.push_back(&unit);
    }

    m_queue.init(m_unitPtrs);
}

Unit BattleSession::makeUnit(const UnitSpawn &spawn)
{
    UnitData data = UnitLoader::load(spawn.unitFilePath);
    data.team = spawn.team;
    const RaceData &raceData = getRaceData(data.race);
    const GenderData &genderData = getGenderData(data.gender);

    return Unit(data, raceData, genderData, spawn.startPos);
}

void BattleSession::spawnUnit(const UnitSpawn &spawn)
{
    assert(m_units.size() < m_units.capacity() &&
           "Unit capacity exceeded — increase UNIT_CAPACITY");

    Unit &unit = buildUnit(spawn);
    m_unitPtrs.push_back(&unit);
    float speed = static_cast<float>(unit.getSpeed());
    float timeCost = TurnQueue::BASE_MOVE_AND_ACTION_COST / speed;

    m_queue.insert(&unit, timeCost);

    if (spawn.isCritical)
        m_criticalUnits.push_back(&unit);
    if (spawn.isBoss)
        m_bossUnits.push_back(&unit);
}

void BattleSession::replaceUnit(Unit *dead, const UnitSpawn &spawn)
{
    assert(dead != nullptr);
    assert(dead->isDead());

    auto it = std::find_if(m_units.begin(), m_units.end(),
                           [dead](const Unit &u)
                           { return &u == dead; });

    assert(it != m_units.end());

    // Remove from critical/boss lists if it was registered
    auto removeFn = [dead](Unit *u)
    { return u == dead; };
    m_criticalUnits.erase(std::remove_if(m_criticalUnits.begin(), m_criticalUnits.end(), removeFn), m_criticalUnits.end());
    m_bossUnits.erase(std::remove_if(m_bossUnits.begin(), m_bossUnits.end(), removeFn), m_bossUnits.end());

    // Remove old unit from timeline before overwriting the slot
    m_queue.remove(dead);

    // Overwrite slot in place — pointer to this slot remains valid
    *it = makeUnit(spawn);

    // Register new unit in critical/boss lists if needed
    if (spawn.isCritical)
        m_criticalUnits.push_back(&(*it));
    if (spawn.isBoss)
        m_bossUnits.push_back(&(*it));

    // Insert into timeline as if it just moved
    float speed = static_cast<float>(it->getSpeed());
    float timeCost = TurnQueue::BASE_MOVE_AND_ACTION_COST / speed;
    m_queue.insert(&(*it), timeCost);
}

void BattleSession::update(float dt)
{
    // Turn logic is event-driven (driven by input in BattleState),
    // not time-driven. Nothing to tick here for now.
    // Future: status effect timers, animations, AI think time.
    (void)dt;
}

void BattleSession::applyDamage(Unit &target, const CombatResult &result)
{
    if (!result.hit)
        return;

    if (result.absorbed)
        target.heal(result.damage);
    else
        target.takeDamage(result.damage);

    // Death already flips the unit's state inside takeDamage() — nothing
    // else to do here. Left as a hook point for wave-mode replacement logic
    // (BattleState is responsible for calling replaceUnit() if wave mode).
}

// ---------------------------------------------------------------------------
// private
// ---------------------------------------------------------------------------

Unit &BattleSession::buildUnit(const UnitSpawn &spawn)
{
    UnitData data = UnitLoader::load(spawn.unitFilePath);
    data.team = spawn.team;
    const RaceData &raceData = getRaceData(data.race);
    const GenderData &genderData = getGenderData(data.gender);

    m_units.emplace_back(data, raceData, genderData, spawn.startPos);
    return m_units.back();
}

void BattleSession::checkBattleResult()
{
    if (m_result != BattleResult::Ongoing)
        return;

    // Defeat checked before victory, same priority order as the old
    // critical-unit-first logic.
    if (evaluateDefeat())
    {
        m_result = BattleResult::Defeat;
        return;
    }

    if (evaluateVictory())
    {
        m_result = BattleResult::Victory;
        return;
    }
}

bool BattleSession::evaluateDefeat() const
{
    switch (m_defeatRule.type)
    {
    case DefeatConditionType::CriticalUnitDied:
        for (const Unit *u : m_criticalUnits)
            if (u->isDead())
                return true;
        return false;
    case DefeatConditionType::AllAlliesDead:
    default:
        return allTeamDead(0); // players are always team 0
    }
}

bool BattleSession::evaluateVictory() const
{
    switch (m_victoryRule.type)
    {
    case VictoryConditionType::KillBoss:
        for (const Unit *u : m_bossUnits)
            if (u->isDead())
                return true;
        return false;
    case VictoryConditionType::SurviveTurns:
        return m_turnsElapsed >= m_victoryRule.turnsToSurvive;
    case VictoryConditionType::KillAll:
    default:
        return allEnemiesDead();
    }
}

bool BattleSession::allTeamDead(int team) const
{
    for (const auto &u : m_units)
    {
        if (u.getTeam() == team && !u.isDead())
            return false;
    }
    return true;
}

// Enemies are team >= 2 in this codebase (see EnemyDefinition::team default
// and BattleCatalog's entries), NOT team 1 — allTeamDead(1) was always
// vacuously true (no unit ever has team==1), meaning the old KillAll check
// would have reported victory instantly, the moment any BattleSession-backed
// battle started. This mirrors BattleState::countAliveEnemies()'s convention.
bool BattleSession::allEnemiesDead() const
{
    for (const auto &u : m_units)
    {
        if (u.getTeam() >= 2 && !u.isDead())
            return false;
    }
    return true;
}