#pragma once

#include "engine/math/Vec2.h"
#include "engine/renderer/Color.h"

#include <string>
#include <vector>

class Renderer;
class Font;
class Camera;

// Queues short-lived floating text (damage numbers, "MISS") anchored to a
// world tile. Mirrors CombatAnimationSystem's enqueue/update/clear shape.
class FloatingTextSystem
{
public:
    struct Entry
    {
        std::string text;
        Vec2i tilePos;
        Color color;
        float ttl = 0.0f;
        float age = 0.0f; // TODO: use for rise/fade easing once rendered
    };

    void enqueue(const std::string &text, Vec2i tilePos, Color color, float ttl = 1.0f);
    void update(float dt);
    // TODO: not implemented yet — needs the same iso/camera transform
    // BattleRenderer uses for units (tileToIso + camera offset/zoom), which
    // this system doesn't have access to. Either give it a Camera& + iso
    // metrics, or move rendering into BattleRenderer::drawScene() where
    // that transform already exists, iterating getEntries() from here.
    void render(Renderer *renderer, const Font *font) const;
    void clear();

    const std::vector<Entry> &getEntries() const { return m_entries; }

private:
    std::vector<Entry> m_entries;
};