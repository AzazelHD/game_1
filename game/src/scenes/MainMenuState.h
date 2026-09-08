#pragma once

#include "engine/scene/Scene.h"
#include "engine/effects/ScreenTransition.h"
#include "ui/UIManager.h"

#include <vector>

class ButtonMenuWindow;
class Font;
class Renderer;

template <typename T>
class StateMachine;

// MainMenuState: show a title screen. Press Enter → push BattleState.
// Phase 5 placeholder — a single line of text is fine initially.
//
// [x]: Implement Scene interface.
//   update(dt): if Input::isKeyPressed(Confirm), transition to BattleState.
//   render(): clear screen to a color, draw "Press Enter" text (or a colored rect for now).
class MainMenuState : public Scene
{
public:
    MainMenuState(StateMachine<Scene> &sm, Renderer *renderer, bool fadeInOnEnter = false);

    void onEnter() override;
    void onExit() override;
    void handleInput() override;
    void update(float dt) override;
    void render(float alpha) override;

private:
    void processUIEvents();
    void openSaveList();
    void refreshSaveList();
    void closeSaveList();
    void refreshMainMenuItems();

private:
    StateMachine<Scene> &m_stateMachine;
    Renderer *m_renderer = nullptr;

    UIManager m_uiManager;
    ScreenTransition m_transition;
    bool m_fadeInOnEnter = false;
    bool m_transitioning = false;

    ButtonMenuWindow *m_mainMenuWindow = nullptr;
    ButtonMenuWindow *m_saveListWindow = nullptr;
    std::vector<int> m_saveListSlotIndexes;
    int m_pendingDeleteSlotIndex = -1;
};
