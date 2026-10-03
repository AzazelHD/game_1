#include "scenes/IntroState.h"

#include "config/GameConstants.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCode.h"
#include "engine/math/MathUtils.h"
#include "engine/renderer/FontManager.h"
#include "engine/renderer/Renderer.h"
#include "engine/statemachine/StateMachine.h"
#include "engine/ui/TextBlock.h"
#include "scenes/BattleState.h"
#include "scenes/WorldMapState.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <memory>
#include <utility>

namespace
{
    constexpr float kLetterboxRatio = 0.17f;
    constexpr Color kSpeakerColor = Color{214, 186, 132, 255};
    constexpr Color kTextColor = Color{226, 232, 244, 255};
} // namespace

IntroState::IntroState(StateMachine<Scene> &sm, Renderer *renderer, std::string scriptPath)
    : m_stateMachine(sm),
      m_renderer(renderer),
      m_scriptPath(std::move(scriptPath))
{
}

void IntroState::onEnter()
{
    m_shotIndex = 0;
    m_shotTime = 0.0f;
    m_ending = false;
    m_transition.reset();
    m_shots.clear();

    if (!loadScript(m_scriptPath))
    {
        // No script (or parse failure) → fall back to a couple of neutral
        // placeholder shots so a fresh game still drops into the world map.
        Shot fallback;
        fallback.top = Color{70, 46, 112, 255};
        fallback.bottom = Color{10, 8, 24, 255};
        fallback.duration = 4.0f;
        fallback.lines.push_back(Line{.speaker = "Narrator", .text = "The frontier has been quiet for too long."});
        fallback.lines.push_back(Line{.speaker = "Narrator", .text = "Somewhere past the treeline, a banner rises..."});
        m_shots.push_back(fallback);
    }

    m_transition.start({
        .transition = ScreenTransitions::FadeIn,
        .duration = 0.8f,
        .easing = easeInOut,
    });
}

void IntroState::onExit()
{
}

void IntroState::handleInput()
{
    if (m_ending)
        return;

    const Input &input = Input::instance();

    if (input.isKeyPressed(KeyCode::Back, false))
    {
        finish();
        return;
    }

    if (input.isKeyPressed(KeyCode::Advance, false) || input.isKeyPressed(KeyCode::Accept, false))
        advance();
}

void IntroState::update(float dt)
{
    m_transition.update(dt);

    if (m_ending)
        return;

    m_shotTime += dt;
    if (m_shotTime >= m_shots[m_shotIndex].duration)
        advance();
}

void IntroState::advance()
{
    if (m_ending)
        return;

    if (m_shotIndex + 1 >= static_cast<int>(m_shots.size()))
    {
        finish();
        return;
    }

    m_shotIndex++;
    m_shotTime = 0.0f;
}

void IntroState::finish()
{
    if (m_ending)
        return;

    m_ending = true;
    // 0.5s out + BattleState's 0.5s fade-in = ~1s hand-off, matching every
    // other scene change in the game.
    m_transition.start({
        .transition = ScreenTransitions::FadeOut,
        .duration = 0.5f,
        .easing = easeInOut,
        .onComplete = [this]()
        {
            // A fresh game drops straight into the tutorial duel after the
            // narrator. Winning (or skipping via the system-menu Quit)
            // continues to the world map; a tutorial defeat retries inside
            // BattleState so a new game can't strand the player at a loss.
            BattleRequest request;
            request.mapPath = "assets/maps/1v1_test.tmj";
            request.returnWorldNodeId = 4;
            request.tutorial = true;
            auto onFinished = [sm = &m_stateMachine, renderer = m_renderer](bool /*won*/)
            {
                sm->replace(std::make_unique<WorldMapState>(*sm, renderer));
            };
            m_stateMachine.replace(std::make_unique<BattleState>(m_stateMachine, m_renderer, request, std::move(onFinished)));
        },
    });
}

bool IntroState::loadScript(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
        return false;

    nlohmann::json root;
    try
    {
        file >> root;
    }
    catch (...)
    {
        return false;
    }

    if (!root.contains("shots") || !root["shots"].is_array())
        return false;

    for (const auto &shotJson : root["shots"])
    {
        Shot shot;

        if (shotJson.contains("top") && shotJson["top"].is_array() && shotJson["top"].size() >= 3)
            shot.top = Color{static_cast<uint8_t>(shotJson["top"][0].get<int>()),
                             static_cast<uint8_t>(shotJson["top"][1].get<int>()),
                             static_cast<uint8_t>(shotJson["top"][2].get<int>()), 255};
        if (shotJson.contains("bottom") && shotJson["bottom"].is_array() && shotJson["bottom"].size() >= 3)
            shot.bottom = Color{static_cast<uint8_t>(shotJson["bottom"][0].get<int>()),
                                static_cast<uint8_t>(shotJson["bottom"][1].get<int>()),
                                static_cast<uint8_t>(shotJson["bottom"][2].get<int>()), 255};

        shot.duration = shotJson.value("duration", 5.0f);

        if (shotJson.contains("lines") && shotJson["lines"].is_array())
        {
            for (const auto &lineJson : shotJson["lines"])
            {
                Line line;
                line.speaker = lineJson.value("speaker", "");
                line.text = lineJson.value("text", "");
                shot.lines.push_back(std::move(line));
            }
        }

        m_shots.push_back(std::move(shot));
    }

    return !m_shots.empty();
}

void IntroState::render(float alpha)
{
    (void)alpha;
    m_renderer->clear(Color{0, 0, 0, 255});

    if (m_shots.empty())
    {
        m_transition.render(m_renderer, GameConstants::VIEW_W, GameConstants::VIEW_H);
        return;
    }

    const Shot &shot = m_shots[m_shotIndex];
    const float t = std::clamp(m_shotTime / std::max(shot.duration, 0.0001f), 0.0f, 1.0f);

    // Gentle push-in: the background grows very slowly across the shot's life.
    const float scale = 1.0f + 0.06f * t;
    const float bgW = GameConstants::VIEW_W * scale;
    const float bgH = GameConstants::VIEW_H * scale;

    m_renderer->setBlendMode(Renderer::BlendMode::None);

    // Vertical gradient (a few horizontal stripes interpolating top → bottom).
    constexpr int kStripes = 48;
    const float topY = (GameConstants::VIEW_H - bgH) * 0.5f;
    for (int i = 0; i < kStripes; ++i)
    {
        const float f = static_cast<float>(i) / static_cast<float>(kStripes - 1);
        const Color color{
            static_cast<uint8_t>(lerp(shot.top.r, shot.bottom.r, f)),
            static_cast<uint8_t>(lerp(shot.top.g, shot.bottom.g, f)),
            static_cast<uint8_t>(lerp(shot.top.b, shot.bottom.b, f)),
            255};
        m_renderer->setDrawColor(color);
        m_renderer->fillRect(Rectf{(GameConstants::VIEW_W - bgW) * 0.5f,
                                   topY + static_cast<float>(i) * (bgH / static_cast<float>(kStripes)),
                                   bgW,
                                   bgH / static_cast<float>(kStripes) + 1.0f});
    }

    // Classic letterbox bars.
    m_renderer->setDrawColor(Color{0, 0, 0, 255});
    const float barH = GameConstants::VIEW_H * kLetterboxRatio;
    m_renderer->fillRect(Rectf{0.0f, 0.0f, GameConstants::VIEW_W, barH});
    m_renderer->fillRect(Rectf{0.0f, GameConstants::VIEW_H - barH, GameConstants::VIEW_W, barH});

    // Text cards sit in the band between the two letterbox bars, anchored to
    // the bottom edge of the band so the block never escapes the visible area.
    const float bandBottom = GameConstants::VIEW_H - barH - 24.0f;
    const float textW = GameConstants::VIEW_W - 220.0f;
    const float textX = (GameConstants::VIEW_W - textW) * 0.5f;
    const Font *bodyFont = FontManager::instance().get(FontRole::Body);
    const Font *headingFont = FontManager::instance().get(FontRole::Heading);
    const Font *titleFont = FontManager::instance().get(FontRole::Title);

    std::vector<TextBlock::Line> textLines;
    textLines.reserve(shot.lines.size() * 2);
    for (const Line &line : shot.lines)
    {
        // A line with no speaker is an emphasis row (title card);
        // speaker + text become two rows: name, then copy.
        const bool emphasis = line.speaker.empty();
        if (!line.speaker.empty())
            textLines.push_back(TextBlock::Line{headingFont, line.speaker, kSpeakerColor});
        if (!line.text.empty())
            textLines.push_back(TextBlock::Line{emphasis ? titleFont : bodyFont, line.text,
                                                emphasis ? kSpeakerColor : kTextColor});
    }

    // Engine text block: bottom-anchored to the band, centered, wrapped to textW.
    TextBlock::render(*m_renderer, textLines, Rectf{textX, barH + 20.0f, textW, bandBottom - barH - 20.0f},
                      TextBlock::Anchor::Bottom, textW, 16.0f, HorizontalAlign::Center);

    m_renderer->setBlendMode(Renderer::BlendMode::Blend);
    m_transition.render(m_renderer, GameConstants::VIEW_W, GameConstants::VIEW_H);
}