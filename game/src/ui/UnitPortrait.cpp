#include "battle/Unit.h"
#include "engine/math/Rect.h"
#include "engine/renderer/Color.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/Renderer.h"
#include "engine/renderer/Texture.h"
#include "ui/UITheme.h"
#include "ui/UIScale.h"
#include "ui/UnitPortrait.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <algorithm>
#include <filesystem>

namespace
{
    constexpr float kTextWidth = 220.0f;
    constexpr float kImageWidth = 50.0f;
    constexpr float kBoxHeight = 116.0f;
    constexpr float kPaddingX = 12.0f;
    constexpr float kNameY = 10.0f;
    constexpr float kHpY = 42.0f;
    constexpr float kMpY = 70.0f;

    std::string resolvePortraitPath(const std::string &spriteSetId)
    {
        if (!spriteSetId.empty())
        {
            std::string candidate = "assets/sprites/units/" + spriteSetId + "/idle.png";
            if (std::filesystem::exists(candidate))
                return candidate;
        }

        const std::string fallback = "assets/sprites/units/standard/idle.png";
        if (std::filesystem::exists(fallback))
            return fallback;
        return std::string();
    }

    void renderText(Renderer *renderer,
                    const Font *font,
                    const std::string &text,
                    Rectf rect,
                    Color color,
                    Renderer::HorizontalAlign hAlign)
    {
        renderer->renderTextInRect(font, text, rect, color, hAlign, Renderer::VerticalAlign::Middle,
                                   false, false, false);
    }
}

Color teamFrameColor(int team)
{
    switch (team)
    {
    case -1:
        return UITheme::NEUTRAL;
    case 0:
        return UITheme::PLAYER;
    case 1:
        return UITheme::ALLY;
    default:
        return UITheme::ENEMY;
    }
}

void UnitPortrait::drawPlaceholderSprite(Renderer *renderer,
                                         const Font *font,
                                         Vec2f center,
                                         float diameter,
                                         int team,
                                         const std::string &letter,
                                         std::uint8_t alpha)
{
    if (!renderer)
        return;

    const float radius = diameter * 0.5f;
    renderer->setBlendMode(Renderer::BlendMode::Blend);
    Color fillColor = teamFrameColor(team);
    fillColor.a = alpha;
    renderer->setDrawColor(fillColor);

    const int r = static_cast<int>(std::ceil(radius));
    for (int y = -r; y <= r; ++y)
    {
        const float dx = std::sqrt(std::max(0.0f, radius * radius - static_cast<float>(y * y)));
        renderer->drawLine(Vec2f{center.x - dx, center.y + static_cast<float>(y)},
                           Vec2f{center.x + dx, center.y + static_cast<float>(y)});
    }

    if (!font || letter.empty())
        return;

    const Vec2f textSize = renderer->measureText(font, letter);
    const Vec2f textPos{center.x - textSize.x * 0.5f, center.y - textSize.y * 0.5f};
    renderer->renderText(font, letter, textPos, Color{255, 255, 255, 255}, true, false, false);
}

Rectf UnitPortrait::render(Renderer *renderer,
                           const Font *font,
                           const Unit &unit,
                           Vec2f topLeft,
                           const PortraitStyle &style)
{
    return renderFromStats(renderer,
                           font,
                           unit.getName(),
                           unit.getLevel(),
                           unit.getCurrentHp(),
                           unit.getMaxHp(),
                           unit.getCurrentMp(),
                           unit.getMaxMp(),
                           topLeft,
                           style,
                           unit.getData().spriteSetId);
}

Rectf UnitPortrait::renderFromStats(Renderer *renderer,
                                    const Font *font,
                                    const std::string &name,
                                    int level,
                                    int currentHp,
                                    int maxHp,
                                    int currentMp,
                                    int maxMp,
                                    Vec2f topLeft,
                                    const PortraitStyle &style,
                                    const std::string &spriteSetId)
{
    if (!renderer || !font)
        return Rectf{topLeft.x, topLeft.y, 0.0f, 0.0f};

    UIScale::refresh();
    const float scale = UIScale::factor();

    const float textW = kTextWidth * scale;
    const float imageW = kImageWidth * scale;
    const float h = kBoxHeight * scale;

    const std::string portraitPath = resolvePortraitPath(spriteSetId);
    const bool hasImage = !portraitPath.empty();

    const float totalW = textW + (hasImage ? imageW : 0.0f);
    const Rectf box{topLeft.x, topLeft.y, totalW, h};

    renderer->setBlendMode(Renderer::BlendMode::Blend);
    renderer->setDrawColor(UITheme::Panel);
    renderer->fillRect(box);
    renderer->setDrawColor(teamFrameColor(style.team));
    renderer->drawRect(box);

    Rectf textRect = box;
    Rectf imageRect{};

    if (hasImage)
    {
        if (style.mirrored)
        {
            textRect = Rectf{box.x, box.y, textW, box.h};
            imageRect = Rectf{box.x + textW, box.y, imageW, box.h};
        }
        else
        {
            imageRect = Rectf{box.x, box.y, imageW, box.h};
            textRect = Rectf{box.x + imageW, box.y, textW, box.h};
        }

        if (Texture *portrait = renderer->loadTexture(portraitPath.c_str()))
        {
            renderer->setTextureScaleMode(portrait, Renderer::ScaleMode::Nearest);
            renderer->setTextureAlphaMod(portrait, style.transparent ? 0.45f : 1.0f);
            const Vec2f texSize = renderer->getTextureSize(portrait);
            renderer->drawTexture(portrait,
                                  Recti{0, 0, static_cast<int>(texSize.x), static_cast<int>(texSize.y)},
                                  Rectf{imageRect.x + 2.0f, imageRect.y + 2.0f, imageRect.w - 4.0f, imageRect.h - 4.0f},
                                  style.mirrored);
            delete portrait;
        }
        else
        {
            // No real sprite on disk yet — placeholder circle+letter sized
            // to fill the image slot, letter drawn larger than the map/hand
            // version to fit the bigger box.
            const float diameter = std::min(imageRect.w, imageRect.h) - 4.0f * scale;
            const Vec2f center{imageRect.x + imageRect.w * 0.5f, imageRect.y + imageRect.h * 0.5f};
            const std::string letter = name.empty() ? std::string() : std::string(1, name[0]);
            drawPlaceholderSprite(renderer, font, center, diameter, style.team, letter);
        }
    }

    const float pad = kPaddingX * scale;
    const float nameY = kNameY * scale;
    const float hpY = kHpY * scale;
    const float mpY = kMpY * scale;
    const float lineH = 20.0f * scale;

    char levelBuf[32];
    char hpBuf[64];
    char mpBuf[64];
    std::snprintf(levelBuf, sizeof(levelBuf), "LV %d", level);
    std::snprintf(hpBuf, sizeof(hpBuf), "HP %d/%d", currentHp, maxHp);
    std::snprintf(mpBuf, sizeof(mpBuf), "MP %d/%d", currentMp, maxMp);

    // Small dedicated slot for the "already placed" marker, right before the
    // level number — only takes up space when actually shown.
    const float markerW = style.showPlacedMarker ? 20.0f * scale : 0.0f;

    if (!style.mirrored)
    {
        renderText(renderer, font, name,
                   Rectf{textRect.x + pad, textRect.y + nameY, textRect.w - 2.0f * pad - 76.0f * scale - markerW, lineH},
                   UITheme::Text,
                   Renderer::HorizontalAlign::Left);
        if (style.showPlacedMarker)
        {
            renderText(renderer, font, "X",
                       Rectf{textRect.x + textRect.w - 76.0f * scale - pad - markerW, textRect.y + nameY, markerW, lineH},
                       Color{220, 60, 60, 255},
                       Renderer::HorizontalAlign::Center);
        }
        renderText(renderer, font, levelBuf,
                   Rectf{textRect.x + textRect.w - 72.0f * scale - pad, textRect.y + nameY, 72.0f * scale, lineH},
                   UITheme::Text,
                   Renderer::HorizontalAlign::Right);
        renderText(renderer, font, hpBuf,
                   Rectf{textRect.x + pad, textRect.y + hpY, textRect.w - 2.0f * pad, lineH},
                   UITheme::Text,
                   Renderer::HorizontalAlign::Left);
        renderText(renderer, font, mpBuf,
                   Rectf{textRect.x + pad, textRect.y + mpY, textRect.w - 2.0f * pad, lineH},
                   UITheme::Text,
                   Renderer::HorizontalAlign::Left);
    }
    else
    {
        renderText(renderer, font, levelBuf,
                   Rectf{textRect.x + pad, textRect.y + nameY, 72.0f * scale, lineH},
                   UITheme::Text,
                   Renderer::HorizontalAlign::Left);
        if (style.showPlacedMarker)
        {
            renderText(renderer, font, "X",
                       Rectf{textRect.x + 76.0f * scale + pad, textRect.y + nameY, markerW, lineH},
                       Color{220, 60, 60, 255},
                       Renderer::HorizontalAlign::Center);
        }
        renderText(renderer, font, name,
                   Rectf{textRect.x + 76.0f * scale + pad + markerW, textRect.y + nameY, textRect.w - 2.0f * pad - 76.0f * scale - markerW, lineH},
                   UITheme::Text,
                   Renderer::HorizontalAlign::Right);
        renderText(renderer, font, hpBuf,
                   Rectf{textRect.x + pad, textRect.y + hpY, textRect.w - 2.0f * pad, lineH},
                   UITheme::Text,
                   Renderer::HorizontalAlign::Right);
        renderText(renderer, font, mpBuf,
                   Rectf{textRect.x + pad, textRect.y + mpY, textRect.w - 2.0f * pad, lineH},
                   UITheme::Text,
                   Renderer::HorizontalAlign::Right);
    }

    return box;
}
