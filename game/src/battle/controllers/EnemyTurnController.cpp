#include "battle/controllers/EnemyTurnController.h"

#include "ai/EnemyAI.h"
#include "battle/BattleSession.h"
#include "battle/unit/Unit.h"

EnemyDecision EnemyTurnController::chooseAction(BattleSession &session, Unit *enemyUnit)
{
    EnemyDecision decision;
    decision.target = EnemyAI::chooseTarget(*enemyUnit, session.getUnitPtrs());

    decision.playersAliveBefore = 0;
    for (const Unit &u : session.getUnits())
    {
        if (!u.isDead() && u.getTeam() == 0)
            ++decision.playersAliveBefore;
    }

    return decision;
}

EnemyTurnResult EnemyTurnController::finishAction(BattleSession &session, int playersAliveBefore)
{
    session.checkResult();

    int playersAliveAfter = 0;
    for (const Unit &u : session.getUnits())
    {
        if (!u.isDead() && u.getTeam() == 0)
            ++playersAliveAfter;
    }

    EnemyTurnResult result;
    result.unitDied = playersAliveAfter < playersAliveBefore;

    switch (session.getResult())
    {
    case BattleResult::Defeat:
        result.outcome = EnemyTurnOutcome::PlayerDefeat;
        break;
    case BattleResult::Victory:
        result.outcome = EnemyTurnOutcome::PlayerVictory;
        break;
    default:
        result.outcome = EnemyTurnOutcome::Continue;
        break;
    }

    return result;
}