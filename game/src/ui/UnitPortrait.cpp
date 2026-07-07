#include "battle/Unit.h"
#include "engine/math/Rect.h"
#include "engine/renderer/Color.h"
#include "engine/renderer/Font.h"
#include "engine/renderer/Renderer.h"
#include "engine/renderer/Texture.h"
#include "ui/UITheme.h"
#include "ui/UIScale.h"
#include "ui/UnitPortrait.h"

#include <filesystem>
#include <cstdio>
#include <string>

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
        renderer->renderTextInRect(font,
                                   text,
                                   rect,
                                   color,
                                   hAlign,
                                   Renderer::VerticalAlign::Middle,
                                   false,
                                   false,
                                   false);
    }
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
    renderer->setDrawColor(style.enemy ? UITheme::Danger : UITheme::Info);
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