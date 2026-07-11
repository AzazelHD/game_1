#pragma once

#include "engine/math/Rect.h"
#include "engine/math/Vec2.h"

#include <string>

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
}