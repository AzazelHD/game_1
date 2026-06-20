#pragma once

class Unit;

// DamagePreview shows predicted damage before the player confirms an attack.
// Shown when the cursor is over a valid attack target.
//
// TODO: Implement the class with:
//   - show(const Unit& attacker, const Unit& target):
//       Call CombatSystem::resolve() but don't apply results — just display them.
//       Store the preview result.
//   - hide()
//   - render(): draw a small box showing "DMG: X | HIT: Y%"
//               If crit is possible, display "CRIT?" warning.
//
// Tip: CombatSystem::resolve() uses rand() — the preview damage won't match the actual
//      roll. For accurate previews, separate the "compute expected value" from the
//      "roll random" step. Something to improve in a later phase.
class DamagePreview
{
public:
    // TODO: implement

private:
    bool m_visible = false;
};
