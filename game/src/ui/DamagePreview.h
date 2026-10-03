#pragma once

#include <string>

class Unit;
class Renderer;
class Font;
class BattleMap;
struct SkillData;

// DamagePreview is the cast confirmation readout.
//
// The player picks a skill, picks a tile, and only *then* does this appear:
// it shows the predicted damage plus a hit-chance gauge for whichever unit in
// the blast the player is currently focusing. Pressing Accept while it is on
// screen casts. It doubles as the confirmation step — there is no separate
// confirm window for battle casts.
//
// [x] Implemented:
//   - show(const Unit& attacker, const Unit& target, const SkillData*) –
//     compute expected damage/hit% through the same HitContext the real
//     resolution uses, and store the numbers separately so the gauge and the
//     text always agree.
//   - hide() – clear the preview.
//   - render(Renderer*, const Font*) – draw damage, an ACC gauge and the
//     percentage.
//
// The preview does NOT roll any random numbers – it uses a deterministic
// formula matching CombatSystem's expected value, so the numbers stay
// consistent while the player cycles targets.
class DamagePreview
{
public:
    // Applies terrain bonuses from attacker/defender tiles into the
    // prediction. `battleMap` may be null to skip terrain bonuses.
    void show(const Unit &attacker, const Unit &target, const SkillData *skill, const BattleMap *battleMap = nullptr);
    void hide();
    bool isVisible() const { return m_visible; }

    // Draw the preview box just below the vertical centre of the screen, so it
    // never covers a focused unit standing in the middle of the view.
    void render(Renderer *renderer, const Font *font) const;

private:
    bool m_visible = false;
    // Damage shown in the readout. Only meaningful when m_dealsDamage — KO,
    // heal, buff and debuff skills show a dash instead of a number.
    int m_damage = 0;
    bool m_dealsDamage = false;
    float m_hitChance = 0.0f;
};