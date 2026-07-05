#pragma once

#include "engine/scene/Scene.h"
#include "engine/statemachine/StateMachine.h"
#include "ui/UIManager.h"

class Renderer;

// PauseState is pushed on top of BattleState when the player pauses.
// Because the state machine uses a stack, BattleState stays in memory underneath.
//
// [x]: Implement Scene interface.
//   onEnter(): build the MenuPanel (Resume / Quit buttons).
//   update(dt): feed input to m_menu; on Resume pop this state → BattleState resumes.
//   render(): draw a semi-transparent overlay, then render m_menu.
//   onExit(): nothing needed here.
//
// Note: BattleState::render() is NOT called while PauseState is on top.
//       The overlay is drawn over a frozen framebuffer — that is intentional.
class PauseState final : public Scene
{
public:
    PauseState(StateMachine<Scene> &sm, Renderer *renderer);
    PauseState(const PauseState &) = delete;
    PauseState &operator=(const PauseState &) = delete;
    PauseState(PauseState &&) = delete;
    PauseState &operator=(PauseState &&) = delete;
    ~PauseState() override = default;
    void onEnter() override;
    void onExit() override;
    void handleInput() override;
    void update(float dt) override;
    void render(float alpha) override;

private:
    void processUIEvents();

private:
    StateMachine<Scene> &m_sm;
    Renderer *m_renderer = nullptr;
    UIManager m_uiManager;
};