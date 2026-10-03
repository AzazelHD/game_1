#pragma once

#include "battle/systems/CombatAnimationSystem.h"

#include <vector>

class Renderer;
class Texture;

// Effect ids that built-in combat VFX answer to. A skill's `visualOnHit`
// names one of these strings, so authoring an animated skill never requires
// touching C++ - only adding the id here if it is a brand new effect.
namespace combat_vfx
{
    // A skull that settles over the target: the "this unit is finished" beat.
    inline constexpr const char *kSkull = "skull";
}

// Owns the textures for every built-in combat VFX and registers them with a
// CombatAnimationSystem.
//
// Adding a new built-in animation is two steps and nothing else:
//   1. drop the texture in assets/sprites/vfx/
//   2. add one row to kBuiltinVisuals in the .cpp
//
// BattleState only holds a library and calls load(); neither it nor the
// resolution controller knows any effect id by name.
class CombatVfxLibrary
{
public:
    CombatVfxLibrary() = default;
    CombatVfxLibrary(const CombatVfxLibrary &) = delete;
    CombatVfxLibrary &operator=(const CombatVfxLibrary &) = delete;

    // Loads every built-in texture and attaches it to `animations` under its
    // effect id. Call once the renderer exists and before the scene renders.
    // A texture that fails to load is warned about and skipped, leaving that
    // effect on the default expanding ring.
    void load(Renderer &renderer, CombatAnimationSystem &animations);

    // Frees the textures. Safe to call more than once.
    void unload();

    ~CombatVfxLibrary() { unload(); }

private:
    std::vector<Texture *> m_textures;
};