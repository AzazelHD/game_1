#pragma once

#include "engine/animation/Tween.h"
#include "engine/math/MathUtils.h"
#include "engine/math/Vec2.h"
#include "engine/renderer/Color.h"
#include "renderer/BattleWorldProjection.h"

#include <string>
#include <vector>

class Renderer;
class Font;

// Queues short-lived floating text (damage numbers, "MISS") anchored to a
// world tile. Each entry owns a Tween<float> that drives both the rise and the
// fade in one eased sweep — expiry comes from the tween finishing instead of
// the hand-rolled TTL list this used to mirror from CombatAnimationSystem.
class FloatingTextSystem
{
public:
    struct Entry
    {
        std::string text;
        Vec2i tilePos;
        Color color;
        float ttl = 1.0f;
        Tween<float> rise; // 0 → 1 over ttl; its eased value drives lift + fade
    };

    void enqueue(const std::string &text, Vec2i tilePos, Color color, float ttl = 1.0f);
    void update(float dt);
    void render(Renderer *renderer, const Font *font, const battleworld::AnchorContext &ctx) const;
    void clear();

    const std::vector<Entry> &getEntries() const { return m_entries; }

private:
    std::vector<Entry> m_entries;
};