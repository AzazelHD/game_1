#pragma once

#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"

#include <string>
#include <cstdint>

class Renderer;
class Font;
class Unit;

namespace UnitPortrait
{
    struct PortraitStyle
    {
        int team = 2;
        bool mirrored = false;
        bool transparent = false;
        bool showPlacedMarker = false;
    };

    Rectf render(Renderer *renderer,
                 const Font *font,
                 const Unit &unit,
                 Vec2f topLeft,
                 const PortraitStyle &style);

    Rectf renderFromStats(Renderer *renderer,
                          const Font *font,
                          const std::string &name,
                          int level,
                          int currentHp,
                          int maxHp,
                          int currentMp,
                          int maxMp,
                          Vec2f topLeft,
                          const PortraitStyle &style,
                          const std::string &spriteSetId = "");

    // Draws a circle + centered initial letter — a generated placeholder
    // sprite used everywhere a real unit image doesn't exist yet (map
    // circles, grabbed-in-hand marker, and the portrait's image slot).
    // `diameter` controls the circle size; the letter is sized relative to it.
    void drawPlaceholderSprite(Renderer *renderer,
                               const Font *font,
                               Vec2f center,
                               float diameter,
                               int team,
                               const std::string &letter,
                               std::uint8_t alpha = 255);
}