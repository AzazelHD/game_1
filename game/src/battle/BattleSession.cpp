#include "battle/BattleSession.h"
#include "data/UnitLoader.h"
#include "battle/UnitProgression.h"
#include <algorithm>
#include <cassert>

// ---------------------------------------------------------------------------
// public
// ---------------------------------------------------------------------------

void BattleSession::init(const std::vector<UnitSpawn> &spawns)
{
    m_units.clear();
    m_units.reserve(UNIT_CAPACITY);
    m_criticalUnits.clear();
    m_bossUnits.clear();
    m_result = BattleResult::Ongoing;

    for (const auto &spawn : spawns)
    {
        Unit &unit = buildUnit(spawn);

        if (spawn.isCritical)
            m_criticalUnits.push_back(&unit);
        if (spawn.isBoss)
            m_bossUnits.push_back(&unit);
    }

    // Collect raw pointers for TurnQueue — safe because m_units never reallocates
    std::vector<Unit *> ptrs;
    ptrs.reserve(m_units.size());
    for (auto &u : m_units)
        ptrs.push_back(&u);

    m_queue.init(ptrs);
}

Unit BattleSession::makeUnit(const UnitSpawn &spawn)
{
    UnitData data = UnitLoader::load(spawn.unitFilePath);
    const RaceData &raceData = getRaceData(data.race);
    const GenderData &genderData = getGenderData(data.gender);

    return Unit(data, raceData, genderData, spawn.startPos);
}

void BattleSession::spawnUnit(const UnitSpawn &spawn)
{
    assert(m_units.size() < m_units.capacity() &&
           "Unit capacity exceeded — increase UNIT_CAPACITY");

    Unit &unit = buildUnit(spawn);
    float speed = static_cast<float>(unit.getSpeed());
    float timeCost = TurnQueue::BASE_MOVE_COST / speed;

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
    float timeCost = TurnQueue::BASE_MOVE_COST / speed;
    m_queue.insert(&(*it), timeCost);
}

void BattleSession::update(float dt)
{
    // Turn logic is event-driven (driven by input in BattleState),
    // not time-driven. Nothing to tick here for now.
    // Future: status effect timers, animations, AI think time.
    (void)dt;
}

void BattleSession::applyResult(Unit &attacker, Unit &target,
                                const CombatResult &result, float turnCost)
{
    // Apply damage or healing
    if (result.hit)
    {
        if (result.absorbed)
            target.heal(result.damage);
        else
            target.takeDamage(result.damage);
    }

    // If target died and disappears, remove from timeline
    if (target.isDead())
    {
        target.setState(UnitState::Dead);

        // NOTE: BattleState is responsible for calling replaceUnit() if wave mode.
        // Here we only handle the disappear-on-death case for normal missions.
        // For persistent dead (zombies etc.), unit stays in timeline as-is.
    }

    // Advance attacker's turn
    m_queue.advance(turnCost);

    // Check win/lose after every action
    checkBattleResult();
}

BattleResult BattleSession::getResult() const
{
    return m_result;
}

// ---------------------------------------------------------------------------
// private
// ---------------------------------------------------------------------------

Unit &BattleSession::buildUnit(const UnitSpawn &spawn)
{
    UnitData data = UnitLoader::load(spawn.unitFilePath);
    const RaceData &raceData = getRaceData(data.race);
    const GenderData &genderData = getGenderData(data.gender);

    m_units.emplace_back(data, raceData, genderData, spawn.startPos);
    return m_units.back();
}

void BattleSession::checkBattleResult()
{
    if (m_result != BattleResult::Ongoing)
        return;

    // Defeat: any critical unit died
    for (const Unit *u : m_criticalUnits)
    {
        if (u->isDead())
        {
            m_result = BattleResult::Defeat;
            return;
        }
    }

    // Victory: any boss unit died
    for (const Unit *u : m_bossUnits)
    {
        if (u->isDead())
        {
            m_result = BattleResult::Victory;
            return;
        }
    }

    // Standard: all enemies dead = victory, all allies dead = defeat
    // Team 0 = players, Team 1 = enemies (by convention)
    if (allTeamDead(1))
    {
        m_result = BattleResult::Victory;
        return;
    }

    if (allTeamDead(0))
    {
        m_result = BattleResult::Defeat;
        return;
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