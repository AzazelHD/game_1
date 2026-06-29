#pragma once

#include "data/SkillLoader.h"
#include "battle/CombatSystem.h"

class Unit;
class Renderer;
class Font;

// DamagePreview shows predicted damage when the cursor is over a valid
// attack target. It is displayed only during the player's attack‑selection
// phase and hidden otherwise.
//
// [x] Implemented:
//   - show(const Unit& attacker, const Unit& target) – compute expected
//     damage, hit%, and crit warning; store results.
//   - hide() – clear the preview.
//   - render(Renderer*, const Font*) – draw a small box with the preview.
//
// The preview does NOT roll any random numbers – it uses a deterministic
// formula matching CombatSystem's expected value, so the numbers stay
// consistent while the player is choosing a target.
class DamagePreview
{
public:
    void show(const Unit &attacker, const Unit &target, const SkillData *skill);
    void hide();
    bool isVisible() const { return m_visible; }

    // Draw the preview box at a fixed screen position (top‑right corner).
    void render(Renderer *renderer, const Font *font) const;

private:
    bool m_visible = false;
    int m_damage = 0;
    float m_hitChance = 100.0f;  // percentage
    bool m_critPossible = false; // reserved for future
};