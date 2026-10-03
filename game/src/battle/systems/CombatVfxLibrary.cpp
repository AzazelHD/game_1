#include "battle/systems/CombatVfxLibrary.h"

#include "engine/core/Log.h"
#include "engine/renderer/Renderer.h"
#include "engine/renderer/Texture.h"

namespace
{
    // One row per built-in combat animation. `scale` is the sprite's width as a
    // fraction of half a tile's width.
    struct BuiltinVisual
    {
        const char *id;    // effect id a skill's `visualOnHit` references
        const char *asset; // filename under assets/sprites/vfx/
        CombatAnimationSystem::SpriteMotion motion;
        float scale;
    };

    constexpr BuiltinVisual kBuiltinVisuals[] = {
        {combat_vfx::kSkull, "skull.png",
         CombatAnimationSystem::SpriteMotion::FadeInHoldRise, 0.6f},
    };

    constexpr const char *kVfxDir = "assets/sprites/vfx/";
} // namespace

void CombatVfxLibrary::load(Renderer &renderer, CombatAnimationSystem &animations)
{
    unload();

    for (const BuiltinVisual &visual : kBuiltinVisuals)
    {
        const std::string path = std::string(kVfxDir) + visual.asset;
        Texture *texture = renderer.loadTexture(path.c_str());
        if (!texture)
        {
            LOG_WARN("Battle", "combat VFX '%s' missing (%s); falls back to a ring",
                     visual.id, path.c_str());
            continue;
        }

        renderer.setTextureScaleMode(texture, Renderer::ScaleMode::Nearest);
        animations.setVisual(visual.id, CombatAnimationSystem::Visual{
                                            .sprite = texture,
                                            .motion = visual.motion,
                                            .tint = Color{255, 255, 255, 255},
                                            .scale = visual.scale});
        m_textures.push_back(texture);
    }
}

void CombatVfxLibrary::unload()
{
    for (Texture *texture : m_textures)
        delete texture;
    m_textures.clear();
}