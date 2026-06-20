#pragma once
#include "engine/scene/Scene.h"
#include "engine/effects/ScreenTransition.h"
#include "engine/statemachine/StateMachine.h"

class Renderer;

// ResultState: shown after a battle ends (win or lose).
// Displays outcome, waits for input, then returns to MainMenuState.
//
// [x]: Implement Scene interface.
//   Constructor should accept a bool (won) or an enum BattleResult.
//   render(): display "VICTORY" or "DEFEAT" and a prompt.
//   update(dt): on Confirm, replace with MainMenuState.
class ResultState final : public Scene
{
public:
    ResultState(StateMachine<Scene> &sm, Renderer *renderer, bool playerWon);

    void onEnter() override;
    void onExit() override;
    void handleInput() override;
    void update(float dt) override;
    void render(float alpha) override;

private:
    StateMachine<Scene> &m_sm;
    Renderer *m_renderer = nullptr;
    bool m_playerWon = false;

    // ── Transition ────────────────────────────────────────────────────────
    ScreenTransition m_transition;
};