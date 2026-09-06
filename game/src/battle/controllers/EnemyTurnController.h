#pragma once

class BattleSession;
class Unit;

// The enemy turn's opening decision: what the acting enemy will do and the
// session snapshot needed to evaluate the action's aftermath.
struct EnemyDecision
{
    Unit *target = nullptr;      // chosen attack target; null if the AI found none
    int playersAliveBefore = 0;  // alive player count snapshot for finishAction()
};

// How the enemy turn resolves from the session's perspective.
enum class EnemyTurnOutcome
{
    Continue,      // battle ongoing — advance to the next unit
    PlayerDefeat,  // players lost — end the battle
    PlayerVictory, // players won — end the battle
};

// Outcome of EnemyTurnController::finishAction(). The scene applies the side
// effects (events, battle-end UI, turn advancement).
struct EnemyTurnResult
{
    EnemyTurnOutcome outcome = EnemyTurnOutcome::Continue;
    bool unitDied = false; // at least one player unit died during this enemy action
};

// EnemyTurnController resolves the aftermath of an enemy unit's action.
// It operates purely on the BattleSession: refreshes the win/lose result,
// detects player-unit deaths since the action began, and reports whether the
// battle ended. The scene keeps responsibility for events, UI, and advancing
// the turn.
class EnemyTurnController
{
public:
    static EnemyDecision chooseAction(BattleSession &session, Unit *enemyUnit);
    static EnemyTurnResult finishAction(BattleSession &session, int playersAliveBefore);
};