#pragma once

#include "engine/effects/ScreenTransition.h"
#include "engine/renderer/Color.h"
#include "engine/scene/Scene.h"

#include <string>
#include <vector>

class Renderer;

template <typename T>
class StateMachine;

// IntroState: plays the short, skippable opening cinematic before a fresh
// campaign. It is driven by assets/intro/script.json — a list of "shots",
// each with background gradient colors, a slow camera push-in, letterbox
// bars, and a set of text cards. Placeholder visuals for now (no story art);
// the format lets you write the real narrative into the JSON without code.
// Press Enter/Space to advance, Esc to skip the whole thing.
class IntroState final : public Scene
{
public:
    IntroState(StateMachine<Scene> &sm, Renderer *renderer,
               std::string scriptPath = "assets/intro/script.json");

    void onEnter() override;
    void onExit() override;
    void handleInput() override;
    void update(float dt) override;
    void render(float alpha) override;

private:
    struct Line
    {
        std::string speaker;
        std::string text;
    };

    struct Shot
    {
        Color top{12, 14, 24, 255};
        Color bottom{2, 3, 8, 255};
        float duration = 5.0f;
        std::vector<Line> lines;
    };

    bool loadScript(const std::string &path);
    void advance();
    void finish();

private:
    StateMachine<Scene> &m_stateMachine;
    Renderer *m_renderer = nullptr;
    std::string m_scriptPath;

    std::vector<Shot> m_shots;
    int m_shotIndex = 0;
    float m_shotTime = 0.0f;

    ScreenTransition m_transition;
    bool m_ending = false;
};